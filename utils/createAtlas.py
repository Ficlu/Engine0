from PIL import Image

def create_texture_atlas(textures, output_path):
    atlas_size = (192, 128)  # 3x2 grid of 64x64 images
    atlas = Image.new('RGBA', atlas_size, (0, 0, 0, 0))  # Transparent background
    
    positions = [(0, 0), (64, 0), (128, 0), (0, 64), (64, 64), (128, 64)]
    
    for texture_path, position in zip(textures, positions):
        with Image.open(texture_path) as texture:
            # Convert to RGBA if not already
            texture = texture.convert('RGBA')
            
            # Replace fully transparent pixels with bright pink
            data = texture.getdata()
            newData = []
            for item in data:
                if item[3] == 0:  # If pixel is fully transparent
                    newData.append((255, 0, 255, 255))  # Bright pink
                else:
                    newData.append(item)
            texture.putdata(newData)
            
            # Paste the modified texture onto the atlas
            atlas.paste(texture, position, texture)
    
    # Convert the final atlas to RGB (removing alpha channel)
    atlas_rgb = Image.new('RGB', atlas.size, (0, 0, 0))
    atlas_rgb.paste(atlas, (0, 0), atlas)
    
    atlas_rgb.save(output_path, 'BMP')

textures = ['Sprite-0001.png', 'Sprite-0002.png', 'rock-0002.png', 'sand-0001.png', 'water-0003.png', 'grass-0004.png']
create_texture_atlas(textures, '../texture_atlas.bmp')