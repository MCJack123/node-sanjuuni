#include <napi.h>
#include <sanjuuni.hpp>

WorkQueue work;
OpenCL::Device * device = NULL;

void FinalizeMat(Napi::Env env, Mat * obj) {delete obj;}
void FinalizeMat1b(Napi::Env env, Mat1b * obj) {delete obj;}

Mat * GetRGBImage(Napi::Env env, Napi::Value value) {
    if (!value.IsObject()) Napi::TypeError::New(env, "RGBImage expected").ThrowAsJavaScriptException();
    Napi::Object obj = value.As<Napi::Object>();
    if (!obj.Get("_obj").IsExternal()) Napi::TypeError::New(env, "RGBImage expected").ThrowAsJavaScriptException();
    return obj.Get("_obj").As<Napi::External<Mat>>().Data();
}

Napi::Object RGBImageAt(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
        Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
    Mat * img = GetRGBImage(env, info.This());
    img->download();
    try {
        const uchar3& color = img->at(info[0].As<Napi::Number>().Uint32Value(), info[0].As<Napi::Number>().Uint32Value());
        Napi::Object retval = Napi::Object::New(env);
        retval.Set("b", Napi::Number::New(env, color.x));
        retval.Set("g", Napi::Number::New(env, color.y));
        retval.Set("r", Napi::Number::New(env, color.z));
        return retval;
    } catch (const std::out_of_range &e) {
        Napi::RangeError::New(env, e.what()).ThrowAsJavaScriptException();
    }
}

Napi::Object NewRGBImage(Napi::Env env, Mat * img) {
    Napi::Object retval = Napi::Object::New(env);
    retval.Set("_obj", Napi::External<Mat>::New(env, img, FinalizeMat));
    retval.Set("width", Napi::Number::New(env, img->width));
    retval.Set("height", Napi::Number::New(env, img->height));
    retval.Set("at", Napi::Function::New(env, RGBImageAt));
    return retval;
}

Mat1b * GetIndexedImage(Napi::Env env, Napi::Value value) {
    if (!value.IsObject()) Napi::TypeError::New(env, "IndexedImage expected").ThrowAsJavaScriptException();
    Napi::Object obj = value.As<Napi::Object>();
    if (!obj.Get("_obji").IsExternal()) Napi::TypeError::New(env, "IndexedImage expected").ThrowAsJavaScriptException();
    return obj.Get("_obji").As<Napi::External<Mat1b>>().Data();
}

Napi::Number IndexedImageAt(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
        Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
    Mat1b * img = GetIndexedImage(env, info.This());
    img->download();
    try {
        uchar color = img->at(info[0].As<Napi::Number>().Uint32Value(), info[0].As<Napi::Number>().Uint32Value());
        return Napi::Number::New(env, color);
    } catch (const std::out_of_range &e) {
        Napi::RangeError::New(env, e.what()).ThrowAsJavaScriptException();
    }
}

Napi::Object NewIndexedImage(Napi::Env env, Mat1b * img) {
    Napi::Object retval = Napi::Object::New(env);
    retval.Set("_obji", Napi::External<Mat1b>::New(env, img, FinalizeMat1b));
    retval.Set("width", Napi::Number::New(env, img->width));
    retval.Set("height", Napi::Number::New(env, img->height));
    retval.Set("at", Napi::Function::New(env, IndexedImageAt));
    return retval;
}

std::vector<Vec3b> GetPalette(Napi::Env env, Napi::Value value) {
    if (!value.IsArray()) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
    Napi::Array array = value.As<Napi::Array>();
    std::vector<Vec3b> retval;
    for (int i = 0; i < array.Length(); i++) {
        Vec3b val;
        Napi::Value v = array.Get(i);
        if (!v.IsObject()) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
        Napi::Object obj = v.As<Napi::Object>();
        Napi::Value rv = obj.Get("b");
        if (!rv.IsNumber()) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
        val[0] = rv.As<Napi::Number>().Uint32Value();
        Napi::Value gv = obj.Get("g");
        if (!gv.IsNumber()) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
        val[1] = gv.As<Napi::Number>().Uint32Value();
        Napi::Value bv = obj.Get("r");
        if (!bv.IsNumber()) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
        val[2] = bv.As<Napi::Number>().Uint32Value();
        retval.push_back(val);
    }
    return retval;
}

Napi::Array NewPalette(Napi::Env env, const std::vector<Vec3b>& palette) {
    Napi::Array retval = Napi::Array::New(env);
    for (int i = 0; i < palette.size(); i++) {
        Napi::Object obj = Napi::Object::New(env);
        obj.Set("b", Napi::Number::New(env, palette[i][0]));
        obj.Set("g", Napi::Number::New(env, palette[i][1]));
        obj.Set("r", Napi::Number::New(env, palette[i][2]));
        retval.Set(i, obj);
    }
    return retval;
}

Napi::Boolean M_initOpenCL(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
#ifdef USE_OPENCL
    if (device != NULL) return Napi::Boolean::New(env, true);
    try {
        OpenCL::Device_Info info;
        std::vector<OpenCL::Device_Info> devices = OpenCL::get_devices(false);
        if (info.Length() > 0) {
            if (info[0].IsNumber()) {
                info = OpenCL::select_device_with_id(info[0].As<Napi::Number>().Uint32Value());
            } else if (info[0].IsString()) {
                std::string str = info[0].As<Napi::String>().Utf8Value();
                if (str == "best_flops") {
                    info = OpenCL::select_device_with_most_flops(devices, false);
                } else if (str == "best_memory") {
                    info = OpenCL::select_device_with_most_memory(devices, false);
                } else {
                    Napi::TypeError::New(env, "Invalid option for device").ThrowAsJavaScriptException();
                }
            } else {
                Napi::TypeError::New(env, "Number or string expected").ThrowAsJavaScriptException();
            }
        } else {
            info = OpenCL::select_device_with_most_flops(devices, false);
        }
        device = new OpenCL::Device(info);
    } catch (const OpenCL::OpenCLException& e) {
        fprintf(stderr, "Failed to initialize OpenCL: %s\n", e.what());
        return Napi::Boolean::New(env, false);
    }
    return Napi::Boolean::New(env, true);
#else
    fprintf(stderr, "Failed to initialize OpenCL: Feature not available\n");
    return Napi::Boolean::New(env, false);
#endif
}

Napi::Object M_makeRGBImage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() == 0)
        Napi::TypeError::New(env, "Object expected").ThrowAsJavaScriptException();
    Mat * img = NULL;
    if (info[0].IsArray()) {
        // Color[][]/[number, number, number][][]
        Napi::Array array = info[0].As<Napi::Array>();
        unsigned height = array.Length();
        if (height == 0) Napi::RangeError::New(env, "Image has no data").ThrowAsJavaScriptException();
        if (!array.Get(0U).IsArray()) Napi::TypeError::New(env, "Invalid pixel array").ThrowAsJavaScriptException();
        unsigned width = array.Get(0U).As<Napi::Array>().Length();
        for (unsigned y = 1; y < height; y++) {
            Napi::Value row = array.Get(y);
            if (!row.IsArray() || row.As<Napi::Array>().Length() != width) Napi::TypeError::New(env, "Invalid pixel array").ThrowAsJavaScriptException();
        }
        img = new Mat(width, height, device);
        for (unsigned y = 0; y < height; y++) {
            Napi::Array row = array.Get(y).As<Napi::Array>();
            Mat::row imgrow = (*img)[y];
            for (unsigned x = 0; x < width; x++) {
                Napi::Value color = row.Get(x);
                if (color.IsArray()) {
                    Napi::Array carr = color.As<Napi::Array>();
                    if (carr.Length() != 3) {
                        delete img;
                        Napi::TypeError::New(env, "Invalid pixel array").ThrowAsJavaScriptException();
                    }
                    if (!carr.Get(0U).IsNumber()) {
                        delete img;
                        Napi::TypeError::New(env, "Invalid pixel array").ThrowAsJavaScriptException();
                    }
                    imgrow[x].z = carr.Get(0U).As<Napi::Number>().Uint32Value();
                    if (!carr.Get(1U).IsNumber()) {
                        delete img;
                        Napi::TypeError::New(env, "Invalid pixel array").ThrowAsJavaScriptException();
                    }
                    imgrow[x].y = carr.Get(1U).As<Napi::Number>().Uint32Value();
                    if (!carr.Get(2U).IsNumber()) {
                        delete img;
                        Napi::TypeError::New(env, "Invalid pixel array").ThrowAsJavaScriptException();
                    }
                    imgrow[x].x = carr.Get(2U).As<Napi::Number>().Uint32Value();
                } else if (color.IsObject()) {
                    Napi::Object carr = color.As<Napi::Object>();
                    if (!carr.Get("r").IsNumber()) {
                        delete img;
                        Napi::TypeError::New(env, "Invalid pixel array").ThrowAsJavaScriptException();
                    }
                    imgrow[x].z = carr.Get("r").As<Napi::Number>().Uint32Value();
                    if (!carr.Get("g").IsNumber()) {
                        delete img;
                        Napi::TypeError::New(env, "Invalid pixel array").ThrowAsJavaScriptException();
                    }
                    imgrow[x].y = carr.Get("g").As<Napi::Number>().Uint32Value();
                    if (!carr.Get("b").IsNumber()) {
                        delete img;
                        Napi::TypeError::New(env, "Invalid pixel array").ThrowAsJavaScriptException();
                    }
                    imgrow[x].x = carr.Get("b").As<Napi::Number>().Uint32Value();
                } else {
                    delete img;
                    Napi::TypeError::New(env, "Invalid pixel array").ThrowAsJavaScriptException();
                }
            }
        }
    } else if (info[0].IsArrayBuffer()) {
        // ArrayBuffer
        Napi::ArrayBuffer array = info[0].As<Napi::ArrayBuffer>();
        switch (info.Length()) {
            case 1: case 2: Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
            case 3: Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
        }
        if (!info[1].IsNumber() || !info[2].IsNumber())
            Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
        if (!info[3].IsString())
            Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
        unsigned width = info[1].As<Napi::Number>().Uint32Value(),
                 height = info[2].As<Napi::Number>().Uint32Value();
        std::string format = info[3].As<Napi::String>().Utf8Value();
        img = new Mat(width, height, device);
        const uint8_t* data = (const uint8_t*)array.Data();
        if (format == "rgb" || format == "bgr") {
            bool bgr = format == "bgr";
            for (unsigned y = 0; y < height; y++) {
                Mat::row row = (*img)[y];
                for (unsigned x = 0; x < width; x++) {
                    unsigned pos = (y * width + x) * 3;
                    if (pos + 2 >= array.ByteLength()) {
                        delete img;
                        Napi::RangeError::New(env, "Image data too short for specified size").ThrowAsJavaScriptException();
                    }
                    if (!bgr) row[x] = {data[pos + 2], data[pos + 1], data[pos]};
                    else row[x] = {data[pos], data[pos + 1], data[pos + 2]};
                }
            }
        } else {
            int tp;
            if (format == "rgba") tp = 0;
            else if (format == "argb") tp = 1;
            else if (format == "bgra") tp = 2;
            else if (format == "abgr") tp = 3;
            else {
                delete img;
                Napi::TypeError::New(env, "Invalid format specification").ThrowAsJavaScriptException();
            }
            for (unsigned y = 0; y < height; y++) {
                Mat::row row = (*img)[y];
                for (unsigned x = 0; x < width; x++) {
                    unsigned pos = (y * width + x) * 4;
                    if (pos + 3 >= array.ByteLength()) {
                        delete img;
                        Napi::RangeError::New(env, "Image data too short for specified size").ThrowAsJavaScriptException();
                    }
                    switch (tp) {
                        case 0: row[x] = {data[pos + 2], data[pos + 1], data[pos]}; break;
                        case 1: row[x] = {data[pos + 3], data[pos + 2], data[pos + 1]}; break;
                        case 2: row[x] = {data[pos], data[pos + 1], data[pos + 2]}; break;
                        case 3: row[x] = {data[pos + 1], data[pos + 2], data[pos + 3]}; break;
                    }
                }
            }
        }
    } else if (info[0].IsBuffer()) {
        // Buffer
        Napi::Buffer<uint8_t> buffer = info[0].As<Napi::Buffer<uint8_t>>();
        switch (info.Length()) {
            case 1: case 2: Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
            case 3: Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
        }
        if (!info[1].IsNumber() || !info[2].IsNumber())
            Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
        if (!info[3].IsString())
            Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
        unsigned width = info[1].As<Napi::Number>().Uint32Value(),
                 height = info[2].As<Napi::Number>().Uint32Value();
        std::string format = info[3].As<Napi::String>().Utf8Value();
        img = new Mat(width, height, device);
        const uint8_t* data = buffer.Data();
        if (format == "rgb" || format == "bgr") {
            bool bgr = format == "bgr";
            for (unsigned y = 0; y < height; y++) {
                Mat::row row = (*img)[y];
                for (unsigned x = 0; x < width; x++) {
                    unsigned pos = (y * width + x) * 3;
                    if (pos + 2 >= buffer.ByteLength()) {
                        delete img;
                        Napi::RangeError::New(env, "Image data too short for specified size").ThrowAsJavaScriptException();
                    }
                    if (!bgr) row[x] = {data[pos + 2], data[pos + 1], data[pos]};
                    else row[x] = {data[pos], data[pos + 1], data[pos + 2]};
                }
            }
        } else {
            int tp;
            if (format == "rgba") tp = 0;
            else if (format == "argb") tp = 1;
            else if (format == "bgra") tp = 2;
            else if (format == "abgr") tp = 3;
            else {
                delete img;
                Napi::TypeError::New(env, "Invalid format specification").ThrowAsJavaScriptException();
            }
            for (unsigned y = 0; y < height; y++) {
                Mat::row row = (*img)[y];
                for (unsigned x = 0; x < width; x++) {
                    unsigned pos = (y * width + x) * 4;
                    if (pos + 3 >= buffer.ByteLength()) {
                        delete img;
                        Napi::RangeError::New(env, "Image data too short for specified size").ThrowAsJavaScriptException();
                    }
                    switch (tp) {
                        case 0: row[x] = {data[pos + 2], data[pos + 1], data[pos]}; break;
                        case 1: row[x] = {data[pos + 3], data[pos + 2], data[pos + 1]}; break;
                        case 2: row[x] = {data[pos], data[pos + 1], data[pos + 2]}; break;
                        case 3: row[x] = {data[pos + 1], data[pos + 2], data[pos + 3]}; break;
                    }
                }
            }
        }
    } else if (info[0].IsTypedArray()) {
        // Uint8Array/Uint32Array
        Napi::TypedArray tarray = info[0].As<Napi::TypedArray>();
        if (tarray.TypedArrayType() == napi_uint8_array) {
            Napi::TypedArrayOf<uint8_t> array = tarray.As<Napi::TypedArrayOf<uint8_t>>();
            switch (info.Length()) {
                case 1: case 2: Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
                case 3: Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
            }
            if (!info[1].IsNumber() || !info[2].IsNumber())
                Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
            if (!info[3].IsString())
                Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
            unsigned width = info[1].As<Napi::Number>().Uint32Value(),
                     height = info[2].As<Napi::Number>().Uint32Value();
            std::string format = info[3].As<Napi::String>().Utf8Value();
            img = new Mat(width, height, device);
            const uint8_t* data = array.Data();
            if (format == "rgb" || format == "bgr") {
                bool bgr = format == "bgr";
                for (unsigned y = 0; y < height; y++) {
                    Mat::row row = (*img)[y];
                    for (unsigned x = 0; x < width; x++) {
                        unsigned pos = (y * width + x) * 3;
                        if (pos + 2 >= array.ByteLength()) {
                            delete img;
                            Napi::RangeError::New(env, "Image data too short for specified size").ThrowAsJavaScriptException();
                        }
                        if (!bgr) row[x] = {data[pos + 2], data[pos + 1], data[pos]};
                        else row[x] = {data[pos], data[pos + 1], data[pos + 2]};
                    }
                }
            } else {
                int tp;
                if (format == "rgba") tp = 0;
                else if (format == "argb") tp = 1;
                else if (format == "bgra") tp = 2;
                else if (format == "abgr") tp = 3;
                else {
                    delete img;
                    Napi::TypeError::New(env, "Invalid format specification").ThrowAsJavaScriptException();
                }
                for (unsigned y = 0; y < height; y++) {
                    Mat::row row = (*img)[y];
                    for (unsigned x = 0; x < width; x++) {
                        unsigned pos = (y * width + x) * 4;
                        if (pos + 3 >= array.ByteLength()) {
                            delete img;
                            Napi::RangeError::New(env, "Image data too short for specified size").ThrowAsJavaScriptException();
                        }
                        switch (tp) {
                            case 0: row[x] = {data[pos + 2], data[pos + 1], data[pos]}; break;
                            case 1: row[x] = {data[pos + 3], data[pos + 2], data[pos + 1]}; break;
                            case 2: row[x] = {data[pos], data[pos + 1], data[pos + 2]}; break;
                            case 3: row[x] = {data[pos + 1], data[pos + 2], data[pos + 3]}; break;
                        }
                    }
                }
            }
        } else if (tarray.TypedArrayType() == napi_uint32_array) {
            Napi::TypedArrayOf<uint32_t> array = tarray.As<Napi::TypedArrayOf<uint32_t>>();
            switch (info.Length()) {
                case 1: case 2: Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
                case 3: Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
            }
            if (!info[1].IsNumber() || !info[2].IsNumber())
                Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
            if (!info[3].IsString())
                Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
            unsigned width = info[1].As<Napi::Number>().Uint32Value(),
                     height = info[2].As<Napi::Number>().Uint32Value();
            std::string format = info[3].As<Napi::String>().Utf8Value();
            img = new Mat(width, height, device);
            const uint32_t* data = array.Data();
            int tp;
            if (format == "rgba") tp = 0;
            else if (format == "argb") tp = 1;
            else if (format == "bgra") tp = 2;
            else if (format == "abgr") tp = 3;
            else {
                delete img;
                Napi::TypeError::New(env, "Invalid format specification").ThrowAsJavaScriptException();
            }
            for (unsigned y = 0; y < height; y++) {
                Mat::row row = (*img)[y];
                for (unsigned x = 0; x < width; x++) {
                    unsigned pos = y * width + x;
                    if (pos >= array.ByteLength()) {
                        delete img;
                        Napi::RangeError::New(env, "Image data too short for specified size").ThrowAsJavaScriptException();
                    }
                    uint32_t d = data[pos];
                    switch (tp) {
                        case 0: row[x] = {(uint8_t)((d >> 8) & 0xFF), (uint8_t)((d >> 16) & 0xFF), (uint8_t)((d >> 24) & 0xFF)}; break;
                        case 1: row[x] = {(uint8_t)((d) & 0xFF), (uint8_t)((d >> 8) & 0xFF), (uint8_t)((d >> 16) & 0xFF)}; break;
                        case 2: row[x] = {(uint8_t)((d >> 24) & 0xFF), (uint8_t)((d >> 16) & 0xFF), (uint8_t)((d >> 8) & 0xFF)}; break;
                        case 3: row[x] = {(uint8_t)((d >> 16) & 0xFF), (uint8_t)((d >> 8) & 0xFF), (uint8_t)((d) & 0xFF)}; break;
                    }
                }
            }
        } else {
            Napi::TypeError::New(env, "Unknown typed array type").ThrowAsJavaScriptException();
        }
    } else {
        Napi::TypeError::New(env, "Object expected").ThrowAsJavaScriptException();
    }

    return NewRGBImage(env, img);
}

Napi::Object M_makeLabImage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) Napi::TypeError::New(env, "RGBImage expected").ThrowAsJavaScriptException();
    return NewRGBImage(env, new Mat(makeLabImage(*GetRGBImage(env, info[0]), device)));
}

Napi::Object M_convertLabPalette(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
    return NewPalette(env, convertLabPalette(GetPalette(env, info[0])));
}

Napi::Object M_reducePalette_medianCut(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) Napi::TypeError::New(env, "RGBImage expected").ThrowAsJavaScriptException();
    int numColors = 16;
    if (info.Length() >= 2) {
        if (!info[1].IsNumber()) Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
        numColors = info[1].As<Napi::Number>().Int32Value();
    }
    return NewPalette(env, reducePalette_medianCut(*GetRGBImage(env, info[0]), numColors, device));
}

Napi::Object M_reducePalette_kMeans(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) Napi::TypeError::New(env, "RGBImage expected").ThrowAsJavaScriptException();
    int numColors = 16;
    if (info.Length() >= 2) {
        if (!info[1].IsNumber()) Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
        numColors = info[1].As<Napi::Number>().Int32Value();
    }
    return NewPalette(env, reducePalette_kMeans(*GetRGBImage(env, info[0]), numColors, device));
}

Napi::Object M_reducePalette_octree(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) Napi::TypeError::New(env, "RGBImage expected").ThrowAsJavaScriptException();
    int numColors = 16;
    if (info.Length() >= 2) {
        if (!info[1].IsNumber()) Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
        numColors = info[1].As<Napi::Number>().Int32Value();
    }
    return NewPalette(env, reducePalette_octree(*GetRGBImage(env, info[0]), numColors, device));
}

Napi::Object M_thresholdImage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() == 0) Napi::TypeError::New(env, "RGBImage expected").ThrowAsJavaScriptException();
    else if (info.Length() == 1) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
    Mat * img = GetRGBImage(env, info[0]);
    std::vector<Vec3b> palette = GetPalette(env, info[1]);
    Mat res = thresholdImage(*img, palette, device);
    return NewIndexedImage(env, new Mat1b(rgbToPaletteImage(res, palette, device)));
}

Napi::Object M_ditherImage_ordered(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() == 0) Napi::TypeError::New(env, "RGBImage expected").ThrowAsJavaScriptException();
    else if (info.Length() == 1) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
    Mat * img = GetRGBImage(env, info[0]);
    std::vector<Vec3b> palette = GetPalette(env, info[1]);
    Mat res = ditherImage_ordered(*img, palette, device);
    return NewIndexedImage(env, new Mat1b(rgbToPaletteImage(res, palette, device)));
}

Napi::Object M_ditherImage_floydSteinberg(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() == 0) Napi::TypeError::New(env, "RGBImage expected").ThrowAsJavaScriptException();
    else if (info.Length() == 1) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
    Mat * img = GetRGBImage(env, info[0]);
    std::vector<Vec3b> palette = GetPalette(env, info[1]);
    Mat res = ditherImage(*img, palette, device);
    return NewIndexedImage(env, new Mat1b(rgbToPaletteImage(res, palette, device)));
}

Napi::String M_makeTable(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() == 0) Napi::TypeError::New(env, "IndexedImage expected").ThrowAsJavaScriptException();
    else if (info.Length() == 1) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
    Mat1b * img = GetIndexedImage(env, info[0]);
    std::vector<Vec3b> palette = GetPalette(env, info[1]);
    uchar *chars, *cols;
    makeCCImage(*img, palette, &chars, &cols, device);
    std::string retval = makeTable(chars, cols, palette, img->width / 2, img->height / 3, info.Length() >= 2 && info[2].ToBoolean(), info.Length() >= 3 && info[3].ToBoolean(), info.Length() >= 4 && info[4].ToBoolean());
    delete[] chars;
    delete[] cols;
    return Napi::String::New(env, retval);
}

Napi::String M_makeNFP(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() == 0) Napi::TypeError::New(env, "IndexedImage expected").ThrowAsJavaScriptException();
    else if (info.Length() == 1) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
    Mat1b * img = GetIndexedImage(env, info[0]);
    std::vector<Vec3b> palette = GetPalette(env, info[1]);
    uchar *chars, *cols;
    makeCCImage(*img, palette, &chars, &cols, device);
    std::string retval = makeNFP(chars, cols, palette, img->width / 2, img->height / 3);
    delete[] chars;
    delete[] cols;
    return Napi::String::New(env, retval);
}

Napi::String M_makeLuaFile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() == 0) Napi::TypeError::New(env, "IndexedImage expected").ThrowAsJavaScriptException();
    else if (info.Length() == 1) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
    Mat1b * img = GetIndexedImage(env, info[0]);
    std::vector<Vec3b> palette = GetPalette(env, info[1]);
    uchar *chars, *cols;
    makeCCImage(*img, palette, &chars, &cols, device);
    std::string retval = makeLuaFile(chars, cols, palette, img->width / 2, img->height / 3);
    delete[] chars;
    delete[] cols;
    return Napi::String::New(env, retval);
}

Napi::String M_makeRawImage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() == 0) Napi::TypeError::New(env, "IndexedImage expected").ThrowAsJavaScriptException();
    else if (info.Length() == 1) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
    Mat1b * img = GetIndexedImage(env, info[0]);
    std::vector<Vec3b> palette = GetPalette(env, info[1]);
    uchar *chars, *cols;
    makeCCImage(*img, palette, &chars, &cols, device);
    std::string retval = makeRawImage(chars, cols, palette, img->width / 2, img->height / 3);
    delete[] chars;
    delete[] cols;
    return Napi::String::New(env, retval);
}

Napi::Buffer<uint8_t> M_make32vid(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() == 0) Napi::TypeError::New(env, "IndexedImage expected").ThrowAsJavaScriptException();
    else if (info.Length() == 1) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
    Mat1b * img = GetIndexedImage(env, info[0]);
    std::vector<Vec3b> palette = GetPalette(env, info[1]);
    uchar *chars, *cols;
    makeCCImage(*img, palette, &chars, &cols, device);
    std::string retval = make32vid(chars, cols, palette, img->width / 2, img->height / 3);
    delete[] chars;
    delete[] cols;
    return Napi::Buffer<uint8_t>::Copy(env, (const uint8_t*)retval.c_str(), retval.size());
}

Napi::Buffer<uint8_t> M_make32vid_cmp(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() == 0) Napi::TypeError::New(env, "IndexedImage expected").ThrowAsJavaScriptException();
    else if (info.Length() == 1) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
    Mat1b * img = GetIndexedImage(env, info[0]);
    std::vector<Vec3b> palette = GetPalette(env, info[1]);
    uchar *chars, *cols;
    makeCCImage(*img, palette, &chars, &cols, device);
    std::string retval = make32vid_cmp(chars, cols, palette, img->width / 2, img->height / 3);
    delete[] chars;
    delete[] cols;
    return Napi::Buffer<uint8_t>::Copy(env, (const uint8_t*)retval.c_str(), retval.size());
}

Napi::Buffer<uint8_t> M_make32vid_ans(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() == 0) Napi::TypeError::New(env, "IndexedImage expected").ThrowAsJavaScriptException();
    else if (info.Length() == 1) Napi::TypeError::New(env, "Palette expected").ThrowAsJavaScriptException();
    Mat1b * img = GetIndexedImage(env, info[0]);
    std::vector<Vec3b> palette = GetPalette(env, info[1]);
    uchar *chars, *cols;
    makeCCImage(*img, palette, &chars, &cols, device);
    std::string retval = make32vid_ans(chars, cols, palette, img->width / 2, img->height / 3);
    delete[] chars;
    delete[] cols;
    return Napi::Buffer<uint8_t>::Copy(env, (const uint8_t*)retval.c_str(), retval.size());
}

void Cleanup() {
    if (device != NULL) delete device;
}

#define addFunction(name) exports.Set(#name, Napi::Function::New(env, M_ ## name))
Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    addFunction(initOpenCL);
    addFunction(makeRGBImage);
    addFunction(makeLabImage);
    addFunction(convertLabPalette);
    addFunction(reducePalette_medianCut);
    addFunction(reducePalette_kMeans);
    addFunction(reducePalette_octree);
    addFunction(thresholdImage);
    addFunction(ditherImage_ordered);
    addFunction(ditherImage_floydSteinberg);
    addFunction(makeTable);
    addFunction(makeNFP);
    addFunction(makeLuaFile);
    addFunction(makeRawImage);
    addFunction(make32vid);
    addFunction(make32vid_cmp);
    addFunction(make32vid_ans);
    env.AddCleanupHook(Cleanup);
    return exports;
}
  
NODE_API_MODULE(sanjuuni, InitAll)
