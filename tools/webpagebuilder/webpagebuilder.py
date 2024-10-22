""" 
Copyright Production 3000

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. 
"""

# Python script to rearrange HTML/CSS/JS into a single line and removing comments and double white spaces, yielding a 40% reduction
#   Usage: python webpagebuilder.py <options> <input_html_file>
#   No arguments default to index.html and full build
#   Options:
#       -l: Embed linked styles, javascript and images
#       -m: Minify the content
#       -e: Encode for ESPAsyncWebServer templating
#       -w: Write the output to ../webpage.h
#       -x: All of the above
#   Output: <filename>.out.<ext>

import base64
import os
import re
import sys


def embed_linked(content : str) -> str:

    # Find linked CSS files, <link rel='stylesheet' type='text/css' href='FILE'>
    css_files = re.findall(r"<link rel='stylesheet' type='text/css' href='(.+?)'>", content)
    for css_file in css_files:
        with open(css_file, 'r') as file:
            print(f"Including: {css_file}")
            css_content = file.read()
            content = content.replace(f"<link rel='stylesheet' type='text/css' href='{css_file}'>", f"<style>\n{css_content}\n</style>")

    # Find linked JS files, <script src='FILE'></script>
    js_files = re.findall(r"<script src='(.+?)'></script>", content)
    for js_file in js_files:
        with open(js_file, 'r') as file:
            print(f"Including: {js_file}")
            js_content = file.read()
            content = content.replace(f"<script src='{js_file}'></script>", f"<script>\n{js_content}\n</script>")

    # Find image files, <img src='FILE' ... tag open to allow for attributes
    img_files = re.findall(r"<img src='(.+?)'", content)
    for img_file in img_files:
        with open(img_file, 'rb') as file:
            print(f"Including: {img_file}")
            img_base64 = base64.b64encode(file.read()).decode('utf-8')
            content = content.replace(f"<img src='{img_file}'", f"<img src='data:image/png;base64,{img_base64}'")

    return content


def minify(content : str) -> str:
        
    original_size = len(content)

    # Remove javascript comments // and /* */
    content = re.sub(r"// .*", "", content) # Note the [space] after // to avoid removing URLs
    content = re.sub(r"/\*.*?\*/", "", content, flags=re.DOTALL)
    # Remove HTML comments <!-- -->
    content = re.sub(r"<!--.*?-->", "", content, flags=re.DOTALL)

    # Remove newlines, reduce spaces to single space
    content = content.replace("\n", "")
    content = " ".join(content.split())

    # Append a single new line, easier for copy paste
    content += "\n"

    # Print simple statistics of file size reduction
    minified_size = len(content)
    print(f"Reduction by {minified_size - original_size} bytes ({100 - (minified_size / original_size * 100):.0f}%) to {minified_size} bytes")

    return content
        

def encode_for_esp(content : str) -> str:
    # Replace all % with %% for ESPAsyncWebServer templating
    content = content.replace("%", "%%")

    print("Encoding % to %% for ESPAsyncWebServer")

    return content


def export_content(content : str):
    # Overwrite the content in the webpage.h file in the parent folder ../webpage.h
    with open("../webpage.h", 'r') as file: # This should be possible with a single open for read/write, but ...
        target = file.read()
        file.close()

        # Find and replace the content, needs to be done in two steps due to the content containing regexy stuff like \s
        target = re.sub(r"===\(.*?\)===", "===(\n)===", target, flags=re.DOTALL)
        target = target.replace(")===", f"{content})===")

        with open("../webpage.h", 'w') as file:
            file.write(target)
            print("Content exported into ../webpage.h")


def save_output(content : str, input_file : str):
    # Create the output file path
    base, ext = os.path.splitext(input_file)
    output_file = f"{base}.out{ext}"
    # Write the content to the output file
    with open(output_file, 'w') as file:
        file.write(content)
    print(f"Output saved: {output_file}")
    

if __name__ == "__main__":
    if len(sys.argv) not in [1, 3]:
        print("Usage: python webpagebuilder.py [lmewx] [input_file]")
    else:
        if len(sys.argv) == 1: # Default to index.html and full build
            input_file = "index.html"
            sys.argv.append("x")
        else:
            input_file = sys.argv[2]

        do_embed = True if "l" or "x" in sys.argv[1] else False
        do_minify = True if "m" or "x" in sys.argv[1] else False
        do_encode = True if "e" or "x" in sys.argv[1] else False
        do_write = True if "w" or "x" in sys.argv[1] else False

        try:
            with open(input_file, 'r') as file:
                print(f"Processing {input_file}")
                content = file.read()

                if do_embed:
                    content = embed_linked(content)
                if do_minify:
                    content = minify(content)
                if do_encode:
                    content = encode_for_esp(content)

                if do_write:
                    export_content(content)
                else:
                    save_output(content, input_file)


        except Exception as e:
            print(f"An error occurred: {e}")
