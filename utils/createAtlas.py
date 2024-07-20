from PIL import Image

def create_texture_atlas(textures, output_path):
    atlas_size = (384, 256)
    atlas = Image.new('RGB', atlas_size)
    
    positions = [(0,0), (0,128), (128,0), (128,128),(256,0),(256,128)]
    
    for texture_path, position in zip(textures, positions):
        texture = Image.open(texture_path)
        atlas.paste(texture, position)
    
    atlas.save(output_path, 'BMP')

textures = ['Sprite-0001.png', 'Sprite-0002.png', 'rock-0002.png','sand-0001.png','water-0003.png','grass-0004.png']
create_texture_atlas(textures, '../texture_atlas.bmp')