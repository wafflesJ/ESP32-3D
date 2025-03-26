import sys
from plyfile import PlyData
import os

def ply_to_txt(ply_path):
    try:
        # Load the PLY file
        ply_data = PlyData.read(ply_path)
        
        # Extract vertices and faces
        vertices = ply_data['vertex']
        faces = ply_data['face']

        # Verify the PLY file contains colors
        has_color = all(color in vertices.data.dtype.names for color in ['red', 'green', 'blue'])

        # Prepare the output filename
        output_path = os.path.splitext(ply_path)[0] + ".txt"
        
        with open(output_path, "w") as txt_file:
            for face in faces.data:
                # Extract the indices of the triangle
                indices = face['vertex_indices']
                if len(indices) != 3:
                    continue  # Ignore non-triangular faces

                # Write the three vertices
                for idx in indices:
                    x, y, z = vertices[idx]['x'], vertices[idx]['y'], vertices[idx]['z']
                    txt_file.write(f"{x} {y} {z}\n")

                # Write the color
                if has_color:
                    r, g, b = (
                        vertices[indices[0]]['red'],
                        vertices[indices[0]]['green'],
                        vertices[indices[0]]['blue']
                    )
                    txt_file.write(f"{r} {g} {b}\n")
                else:
                    txt_file.write("255 255 255\n")  # Default to white if no color
        
        print(f"Converted {ply_path} to {output_path}")

    except Exception as e:
        print(f"Error processing {ply_path}: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: ply_to_txt_converter.py <path_to_ply_file>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    ply_to_txt(input_file)
