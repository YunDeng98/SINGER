import argparse
import sys

def index_haps(input_prefix, segment_length, mutation_threshold=100):
    input_file = f"{input_prefix}.haps"
    index_file = f"{input_prefix}.index"
    
    current_segment_start = -1
    byte_offset = 0
    last_variant_segment = -1
    mutation_count = 0  # Track the number of mutations in the current segment
    
    # Clear existing index file
    with open(index_file, 'w') as f:
        f.write("")
    
    # Read .haps file line-by-line
    with open(input_file, 'r') as f:
        for line in f:
            # Extract position (the third space-separated field)
            pos = int(line.split()[2])
            
            # Calculate the start coordinate of the segment this variant belongs to
            segment_start = (pos // segment_length) * segment_length
            
            # If this is the first variant in a new segment
            if segment_start != current_segment_start:
                # Check if the previous segment had enough mutations before saving
                if mutation_count >= mutation_threshold:
                    # Record this segment start and byte offset
                    with open(index_file, 'a') as f_idx:
                        f_idx.write(f"{last_variant_segment}\t{byte_offset}\n")
                
                # Reset mutation count for the new segment
                mutation_count = 0
                last_variant_segment = segment_start
                current_segment_start = segment_start
            
            # Count mutation if there's any non-'0' entry in the genotype data
            if any(x != '0' for x in line.split()[5:]):
                mutation_count += 1

            # Increment byte offset
            byte_offset += len(line.encode('utf-8'))

    # Final check to include the last segment if it meets the mutation threshold
    if mutation_count >= mutation_threshold:
        with open(index_file, 'a') as f_idx:
            f_idx.write(f"{last_variant_segment}\t{byte_offset}\n")


def main():
    parser = argparse.ArgumentParser(description="Index a haps file by block length.")
    parser.add_argument("haps_file_prefix", type=str, help="haps file prefix without .haps extension")
    parser.add_argument("segment_length", type=float, help="Length of segments to index")
    args = parser.parse_args()

    print("Index haps files")
    print("------------------")
    print(f"haps file prefix: {args.haps_file_prefix}")
    print(f"Block length: {args.segment_length}")
    print("------------------")

    index_haps(args.haps_file_prefix, args.segment_length)

if __name__ == "__main__":
    main()

