from PIL import Image

def create_texture_atlas(textures, output_path):
    atlas_size = (1024, 1024)
    atlas = Image.new('RGB', atlas_size)
    
    positions = [(0,0), (512,0)]
    
    for texture_path, position in zip(textures, positions):
        texture = Image.open(texture_path)
        atlas.paste(texture, position)
    
    atlas.save(output_path, 'BMP')

textures = ['texture1.png', 'texture2.png']
create_texture_atlas(textures, '../texture_atlas.bmp')