from PIL import Image

def create_texture_atlas(textures, output_path):
    atlas_size = (1024, 1536)
    atlas = Image.new('RGB', atlas_size)
    
    positions = [(0,0), (512,0), (0,512), (512,512),(0,1024),(512,1024)]
    
    for texture_path, position in zip(textures, positions):
        texture = Image.open(texture_path)
        atlas.paste(texture, position)
    
    atlas.save(output_path, 'BMP')

textures = ['Sprite-0001.png', 'Sprite-0002.png', 'Sprite-0003.png','Sprite-0004.png','Sprite-0005.png','Sprite-0006.png']
create_texture_atlas(textures, '../texture_atlas.bmp')