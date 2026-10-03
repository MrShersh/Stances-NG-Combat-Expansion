using System.IO;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;

// Turns the stance artwork in art/icons into the indicator's icons: white with the artwork's alpha (the indicator tints
// them with the stance colour), cropped to the drawing, centred on a square canvas.
//
// Usage: dotnet run -- analyze <image>...
//        dotnet run -- build <art dir> <output dir> [size]

if (args.Length >= 2 && args[0] == "analyze")
{
    foreach (var path in args.Skip(1))
        Analyze(path);
    return;
}
if (args.Length >= 3 && args[0] == "build")
{
    var size = args.Length > 3 ? int.Parse(args[3]) : 512;
    Directory.CreateDirectory(args[2]);
    foreach (var name in new[] { "bear", "wolf", "hawk" })
    {
        var source = Directory.EnumerateFiles(args[1], name + ".*").FirstOrDefault()
                     ?? throw new FileNotFoundException($"{name}.* not found in {args[1]}");
        var target = Path.Combine(args[2], name + ".png");
        Build(source, target, size);
        Console.WriteLine($"{source} -> {target}");
        Analyze(target);
    }
    return;
}
Console.WriteLine("usage: analyze <image>... | build <art dir> <output dir> [size]");

static (int Width, int Height, byte[] Bgra) Load(string path)
{
    var frame = BitmapDecoder.Create(new Uri(Path.GetFullPath(path)), BitmapCreateOptions.PreservePixelFormat, BitmapCacheOption.OnLoad).Frames[0];
    var bgra = new FormatConvertedBitmap(frame, PixelFormats.Bgra32, null, 0);
    var pixels = new byte[bgra.PixelWidth * bgra.PixelHeight * 4];
    bgra.CopyPixels(pixels, bgra.PixelWidth * 4, 0);
    return (bgra.PixelWidth, bgra.PixelHeight, pixels);
}

static void Analyze(string path)
{
    var (w, h, px) = Load(path);
    var hist = new int[8];
    long rgbSum = 0, visible = 0;
    for (var i = 0; i < px.Length; i += 4)
    {
        hist[px[i + 3] / 32]++;
        if (px[i + 3] > 128)
        {
            rgbSum += px[i] + px[i + 1] + px[i + 2];
            visible++;
        }
    }
    Console.WriteLine($"{Path.GetFileName(path)}: {w}x{h}, avg RGB where alpha>128: {(visible > 0 ? rgbSum / visible / 3 : 0)}");
    Console.WriteLine("  alpha histogram (32 steps): " + string.Join(" ", hist));
}

static void Build(string source, string target, int size)
{
    var (w, h, px) = Load(source);

    // Bounding box of the drawing, ignoring near-invisible noise.
    int minX = w, minY = h, maxX = -1, maxY = -1;
    for (var y = 0; y < h; y++)
        for (var x = 0; x < w; x++)
            if (px[(y * w + x) * 4 + 3] > 8)
            {
                minX = Math.Min(minX, x); maxX = Math.Max(maxX, x);
                minY = Math.Min(minY, y); maxY = Math.Max(maxY, y);
            }
    if (maxX < 0)
        throw new InvalidDataException($"{source} is fully transparent");

    // Alpha only: the colour comes from the indicator's tint.
    var mask = new byte[px.Length];
    for (var i = 0; i < px.Length; i += 4)
    {
        mask[i] = mask[i + 1] = mask[i + 2] = 255;
        mask[i + 3] = px[i + 3];
    }
    var maskBitmap = BitmapSource.Create(w, h, 96, 96, PixelFormats.Bgra32, null, mask, w * 4);
    var cropped = new CroppedBitmap(maskBitmap, new Int32Rect(minX, minY, maxX - minX + 1, maxY - minY + 1));

    // Fit into the canvas with a small margin, keeping the aspect ratio.
    var inner = size * 0.96;
    var scale = Math.Min(inner / cropped.PixelWidth, inner / cropped.PixelHeight);
    var drawW = cropped.PixelWidth * scale;
    var drawH = cropped.PixelHeight * scale;
    var visual = new DrawingVisual();
    RenderOptions.SetBitmapScalingMode(visual, BitmapScalingMode.HighQuality);
    using (var dc = visual.RenderOpen())
        dc.DrawImage(cropped, new Rect((size - drawW) / 2, (size - drawH) / 2, drawW, drawH));
    var render = new RenderTargetBitmap(size, size, 96, 96, PixelFormats.Pbgra32);
    render.Render(visual);

    // Back to straight alpha and pure white, so tinting gives exactly the stance colour.
    var straight = new FormatConvertedBitmap(render, PixelFormats.Bgra32, null, 0);
    var output = new byte[size * size * 4];
    straight.CopyPixels(output, size * 4, 0);
    for (var i = 0; i < output.Length; i += 4)
        output[i] = output[i + 1] = output[i + 2] = 255;

    var encoder = new PngBitmapEncoder();
    encoder.Frames.Add(BitmapFrame.Create(BitmapSource.Create(size, size, 96, 96, PixelFormats.Bgra32, null, output, size * 4)));
    using var file = File.Create(target);
    encoder.Save(file);
}
