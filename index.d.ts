import {Buffer} from "buffer";

export module "sanjuuni" {
    /** A color, usually in RGB, but may also be holding Lab coordinates. */
    type Color = {r: number, g: number, b: number};
    /** A palette of colors. */
    type Palette = Color[];
    /** A palette of Lab colors. */
    type LabPalette = Palette;

    /**
     * Holds an RGB image.
     * Wrapper around sanjuuni `Mat`.
     */
    declare interface RGBImage {
        width: number;
        height: number;
        at(x: number, y: number): Color;
    }
    type LabImage = RGBImage;

    /**
     * Holds an 8-bit indexed image.
     * Wrapper around sanjuuni `Mat1b`.
     */
    declare interface IndexedImage {
        width: number;
        height: number;
        at(x: number, y: number): number;
    }

    /**
     * Initializes OpenCL support if available.
     * @param device The device to use; specify "best_flops" for device with most computation power, or "best_memory" for device with most memory
     * @returns Whether OpenCL initialization succeeded (if it failed, sanjuuni will still work in CPU-only mode)
     */
    declare function initOpenCL(device: number | "best_flops" | "best_memory" = "best_flops"): boolean;

    /**
     * Creates an image from a 2D color array.
     * @param image The image source
     * @returns The new RGB image
     */
    declare function makeRGBImage(image: Color[][] | [number, number, number][][]): RGBImage;
    /**
     * Creates an image from a byte buffer.
     * @param image The image source
     * @param width The width of the image
     * @param height The height of the image
     * @param format The format the data is in, i.e. the byte order
     * @returns The new RGB image
     */
    declare function makeRGBImage(image: Buffer, width: number, height: number, format: "rgb" | "rgba" | "bgr" | "bgra" | "argb" | "abgr"): RGBImage;
    /**
     * Creates an image from a byte buffer.
     * @param image The image source
     * @param width The width of the image
     * @param height The height of the image
     * @param format The format the data is in, i.e. the byte order
     * @returns The new RGB image
     */
    declare function makeRGBImage(image: ArrayBuffer, width: number, height: number, format: "rgb" | "rgba" | "bgr" | "bgra" | "argb" | "abgr"): RGBImage;
    /**
     * Creates an image from a byte buffer.
     * @param image The image source
     * @param width The width of the image
     * @param height The height of the image
     * @param format The format the data is in, i.e. the byte order
     * @returns The new RGB image
     */
    declare function makeRGBImage(image: Uint8Array, width: number, height: number, format: "rgb" | "rgba" | "bgr" | "bgra" | "argb" | "abgr"): RGBImage;
    /**
     * Creates an image from a 32-bit integer buffer.
     * @param image The image source
     * @param width The width of the image
     * @param height The height of the image
     * @param format The format the data is in, i.e. the byte order
     * @returns The new RGB image
     */
    declare function makeRGBImage(image: Uint32Array, width: number, height: number, format: "rgba" | "bgra" | "argb" | "abgr"): RGBImage;

    /**
     * Converts an sRGB image into CIELAB color space.
     * @param image The image to convert
     * @return A new image with all pixels in Lab color space
     */
    declare function makeLabImage(image: RGBImage): LabImage;
    /**
     * Converts a list of Lab colors into sRGB colors.
     * @param palette The colors to convert
     * @return A new list with all colors converted to RGB
     */
    declare function convertLabPalette(palette: LabPalette): Palette;

    /**
     * Generates an optimized palette for an image using the median cut algorithm.
     * @param image The image to generate a palette for
     * @param numColors The number of colors to get (must be a power of 2)
     * @returns An optimized palette for the image
     */
    declare function reducePalette_medianCut(image: RGBImage, numColors: number = 16): Palette;
    /**
     * Generates an optimized palette for an image using the median cut algorithm.
     * @param image The image to generate a palette for
     * @param numColors The number of colors to get (must be a power of 2)
     * @returns An optimized palette for the image
     */
    declare function reducePalette_medianCut(image: LabImage, numColors: number = 16): LabPalette;
    /**
     * Generates an optimized palette for an image using the k-means algorithm.
     * @param image The image to generate a palette for
     * @param numColors The number of colors to get
     * @returns An optimized palette for the image
     */
    declare function reducePalette_kMeans(image: RGBImage, numColors: number = 16): Palette;
    /**
     * Generates an optimized palette for an image using the k-means algorithm.
     * @param image The image to generate a palette for
     * @param numColors The number of colors to get
     * @returns An optimized palette for the image
     */
    declare function reducePalette_kMeans(image: LabImage, numColors: number = 16): LabPalette;
    /**
     * Generates an optimized palette for an image using octrees.
     * @param image The image to generate a palette for
     * @param numColors The number of colors to get
     * @returns An optimized palette for the image
     */
    declare function reducePalette_octree(image: RGBImage, numColors: number = 16): Palette;
    /**
     * Generates an optimized palette for an image using octrees.
     * @param image The image to generate a palette for
     * @param numColors The number of colors to get
     * @returns An optimized palette for the image
     */
    declare function reducePalette_octree(image: LabImage, numColors: number = 16): LabPalette;

    /**
     * Reduces the colors in an image using the specified palette through thresholding.
     * @param image The image to reduce
     * @param palette The palette to use
     * @return A reduced-color version of the image using the palette
     */
    declare function thresholdImage(image: RGBImage, palette: Palette): IndexedImage;
    /**
     * Reduces the colors in an image using the specified palette through thresholding.
     * @param image The image to reduce
     * @param palette The palette to use
     * @return A reduced-color version of the image using the palette
     */
    declare function thresholdImage(image: LabImage, palette: LabPalette): IndexedImage;
    /**
     * Reduces the colors in an image using the specified palette through ordered
     * dithering.
     * @param image The image to reduce
     * @param palette The palette to use
     * @return A reduced-color version of the image using the palette
     */
    declare function ditherImage_ordered(image: RGBImage, palette: Palette): IndexedImage;
    /**
     * Reduces the colors in an image using the specified palette through ordered
     * dithering.
     * @param image The image to reduce
     * @param palette The palette to use
     * @return A reduced-color version of the image using the palette
     */
    declare function ditherImage_ordered(image: LabImage, palette: LabPalette): IndexedImage;
    /**
     * Reduces the colors in an image using the specified palette through Floyd-
     * Steinberg dithering.
     * @param image The image to reduce
     * @param palette The palette to use
     * @return A reduced-color version of the image using the palette
     */
    declare function ditherImage_floydSteinberg(image: RGBImage, palette: Palette): IndexedImage;
    /**
     * Reduces the colors in an image using the specified palette through Floyd-
     * Steinberg dithering.
     * @param image The image to reduce
     * @param palette The palette to use
     * @return A reduced-color version of the image using the palette
     */
    declare function ditherImage_floydSteinberg(image: LabImage, palette: LabPalette): IndexedImage;

    /**
     * Generates a blit table from the specified CC image.
     * @param input The image to convert
     * @param palette The palette for the image
     * @param compact Whether to make the output as compact as possible
     * @param embedPalette Whether to embed the palette as a `palette` key (for BIMG)
     * @return The generated blit image source for the image data
     */
    declare function makeTable(image: IndexedImage, palette: Palette, compact: boolean = false, embedPalette: boolean = false, binary: boolean = false): string;
    /**
     * Generates an NFP image from the specified CC image. This changes proportions!
     * @param input The image to convert
     * @param palette The palette for the image
     * @return The generated NFP for the image data
     */
    declare function makeNFP(image: IndexedImage, palette: Palette): string;
    /**
     * Generates a Lua display script from the specified CC image. This file can be
     * run directly to show the image on-screen.
     * @param input The image to convert
     * @param palette The palette for the image
     * @return The generated  for the image data
     */
    declare function makeLuaFile(image: IndexedImage, palette: Palette): string;
    /**
     * Generates a raw mode frame from the specified CC image.
     * @param input The image to convert
     * @param palette The palette for the image
     * @return The generated  for the image data
     */
    declare function makeRawImage(image: IndexedImage, palette: Palette): string;
    /**
     * Generates an uncompressed 32vid frame from the specified CC image.
     * @param input The image to convert
     * @param palette The palette for the image
     * @return The generated  for the image data
     */
    declare function make32vid(image: IndexedImage, palette: Palette): Buffer;
    /**
     * Generates a 32vid frame from the specified CC image using the custom compression scheme.
     * @param input The image to convert
     * @param palette The palette for the image
     * @return The generated  for the image data
     */
    declare function make32vid_cmp(image: IndexedImage, palette: Palette): Buffer;
    /**
     * Generates a 32vid frame from the specified CC image using the custom ANS compression scheme.
     * @param input The image to convert
     * @param palette The palette for the image
     * @return The generated  for the image data
     */
    declare function make32vid_ans(image: IndexedImage, palette: Palette): Buffer;
}
