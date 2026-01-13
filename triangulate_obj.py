import sys

def triangulate_obj(input_path, output_path):
    print(f"Triangulating {input_path} -> {output_path}")
    with open(input_path, 'r') as f_in, open(output_path, 'w') as f_out:
        for line in f_in:
            if line.startswith('f '):
                parts = line.split()
                # parts[0] is 'f'
                # parts[1:] are vertices
                vertices = parts[1:]
                if len(vertices) > 3:
                    # Triangulate: Fan from the first vertex
                    v1 = vertices[0]
                    for i in range(1, len(vertices) - 1):
                        v2 = vertices[i]
                        v3 = vertices[i+1]
                        f_out.write(f"f {v1} {v2} {v3}\n")
                else:
                    f_out.write(line)
            else:
                f_out.write(line)
    print("Done.")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python triangulate.py input.obj output.obj")
    else:
        triangulate_obj(sys.argv[1], sys.argv[2])
