from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import io

# Try to load a sans serif font; fallback to default
try:
    font = ImageFont.truetype("/System/Library/Fonts/HelveticaNeue.ttc", 250, index=4)
except IOError:
    font = ImageFont.load_default()

codes = ["EN", "FR", "DE", "IT", "ES", "NL", "PL"]
paths = []
for code in codes:
    img = Image.new("RGBA", (512, 390), (255, 255, 255, 0))
    draw = ImageDraw.Draw(img)
    # Draw circle
    draw.ellipse((0, 0, 512, 390), fill=(0,0,0,255))
    # measure text
    bbox = draw.textbbox((0, 0), code, font=font)
    w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
    x = (512 - w) / 2 - bbox[0]
    y = (390 - h) / 2 - bbox[1]
    draw.text((x, y), code, font=font, fill=(255,255,255,255))
    pth = Path(f"bubble_{code}.png")
    img.save(pth)
    paths.append(str(pth))
paths
