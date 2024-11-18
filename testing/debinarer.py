import os
import argparse

def combine_chunks(directory, output_file):
    # List all files in the directory, sorted by filename (assuming filenames are ordered as filename_1, filename_2, ...)
    chunk_files = sorted(os.listdir(directory), key=lambda x: int(x.split('_')[-1]))

    # Open the output file for writing the combined data (in binary mode)
    with open(output_file, 'wb') as output:  # Open output file in binary mode
        for chunk_file in chunk_files:
            chunk_path = os.path.join(directory, chunk_file)

            # Check if the file is indeed a chunk (binary file)
            if os.path.isfile(chunk_path):
                try:
                    with open(chunk_path, 'rb') as chunk:  # Open each chunk in binary mode
                        output.write(chunk.read())  # Write the chunk data to the output file
                except Exception as e:
                    print(f"Error reading chunk {chunk_file}: {e}")
                    continue  # Skip to the next chunk if there's an error

    print(f"File successfully restored to: {output_file}")

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description="Reassemble binary chunks into the original file.")
    parser.add_argument('directory', type=str, help="Directory containing binary chunk files.")
    parser.add_argument('output_file', type=str, help="Output file to restore the original file.")

    # Parse the command line arguments
    args = parser.parse_args()

    # Combine the chunks into the original file
    combine_chunks(args.directory, args.output_file)

if __name__ == "__main__":
    main()
