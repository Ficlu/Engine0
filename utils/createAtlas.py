from PIL import Image

def create_texture_atlas(textures, output_path):
    atlas_size = (96, 64)  # Assuming 32x32 tiles and a 3x2 grid
    atlas = Image.new('RGB', atlas_size)
    
    positions = [(0, 0), (0, 32), (32, 0), (32, 32), (64, 0), (64, 32)]
    
    for texture_path, position in zip(textures, positions):
        texture = Image.open(texture_path).resize((32, 32))
        atlas.paste(texture, position)
    
    atlas.save(output_path, 'BMP')

textures = ['Sprite-001.bmp', 'Sprite-0002.bmp', 'rock-0002.bmp', 'sand-0001.bmp', 'water-0003.bmp', 'grass-0004.bmp']
create_texture_atlas(textures, '../texture_atlas.bmp')
