import os
import pyperclip

def copyAll():
    current_directory = os.getcwd()
    combined_content = ""

    ignore_files = {"glew.c", "glewinfo.c", "visualinfo.c"}

    for filename in os.listdir(current_directory):
        if (os.path.isfile(filename) and (filename.endswith(".h") or filename.endswith(".c"))
                and filename not in ignore_files):
            with open(os.path.join(current_directory, filename), 'r') as file:
                content = file.read()
                combined_content += f"// {filename}\n{content}\n\n"

    pyperclip.copy(combined_content)
    print("All file contents have been copied to the clipboard.")

if __name__ == "__main__":
    copyAll()
