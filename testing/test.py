import random
import string

# Define the size of the target file in bytes (10 MB)
target_size_bytes = 10 * 1024 * 1024  # 10 MB

# Generate random lorem-like text
def generate_lorem_word():
    return ''.join(random.choices(string.ascii_lowercase, k=random.randint(3, 10)))

def generate_lorem_paragraph():
    return ' '.join(generate_lorem_word() for _ in range(random.randint(50, 100))) + '.'

# Open a file to write
filename = "lorem_ipsum_10mb.txt"
with open(filename, "w") as file:
    current_size = 0
    while current_size < target_size_bytes:
        paragraph = generate_lorem_paragraph() + "\n"
        file.write(paragraph)
        current_size += len(paragraph)

filename
