import os
import sys
import subprocess
import shutil
import re

# Updates the "headers.h" file to assign each individual SS a unique port number for NFS and client communication
def update_header_and_config_files(i: int, base_dir: str, cwd: str) -> None:
    # Now assigning the port numbers to this Storage server
    input_file = base_dir + "/headers.h"    # File in which these macros are stored
    # Defining regex to identify the line
    pattern1 = r'^\s*#define\s*MY_NFS_PORT_NO.*$'   
    pattern2 = r'^\s*#define\s*MY_CLIENT_PORT_NO.*$'
    pattern3 = r'^\s*#define\s*MY_SS_ID.*$'

    # Read the content of the input file
    with open(input_file, 'r') as file:
        lines = file.readlines()

    # Modify the lines to replace #define with #ifndef
    modified_lines = [re.sub(pattern1, f'#define MY_NFS_PORT_NO {1000 * (i + 1) + 500}', line) for line in lines]
    modified_lines = [re.sub(pattern2, f'#define MY_CLIENT_PORT_NO {1000 * (i + 1) + 501}', line) for line in modified_lines]
    modified_lines = [re.sub(pattern3, f'#define MY_SS_ID {i + 1}', line) for line in modified_lines]
    
    # Write the modified content back to the same file
    with open(input_file, 'w') as file:
        file.writelines(modified_lines)
        
    with open("ss_config.txt", "a") as config_file:
        config_file.write(f"SS {i + 1}:\nNFS_PORT_NO_SS{i + 1} = {1000 * (i + 1) + 500}\nCLIENT_PORT_NO_SS_{i + 1} = {1000 * (i + 1) + 501}\nSS_ID = {i + 1}\n\n")

# Cleans all the make compiled files
def run_clean(i: int) -> None:
    file_to_be_deleted = f"SS{i + 1}"
    subprocess.run(['rm', '-r', file_to_be_deleted], check = True)
    # os.chdir(f"SS{i + 1}")
    # subprocess.run(['make', 'clean'],  check = True)
    # os.chdir("..")

# Compiles all the make files
def compile_make(i: int) -> None:
    os.chdir(f"SS{i + 1}")
    subprocess.run(['make'],  check = True)
    os.chdir("..")

# Copy the "SS" folder n times (n specified through command line argument)
def copy_folder_n_times(num_of_ss: int, cwd: str) -> None:
    source_dir = cwd + "/SS"

    for i in range(num_of_ss):
        dest_dir = cwd + f"/SS{i + 1}"
        try:
            shutil.copytree(source_dir, dest_dir)
        except FileExistsError as e:
            pass
        finally:
            pass
    
# Creates a directory at the specified absolute path
def create_dir(path: str) -> None:
    # Try to create the directory and if it already exists then just ignore and continue
    try:
        os.mkdir(path)
    except FileExistsError as e:
        pass
    finally:
        pass
    
# Creates a file with the current file index in it's name at the path provided
def create_file(path: str, curr_file_index: int) -> None:
    try:
        with open(path, "x") as file:
            file.write(f"This is sample text in SS {i + 1} and file {curr_file_index}")
    except FileExistsError:
        pass
    
# Clearing the config file
with open("ss_config.txt", "w") as config_file:
    pass

# Setting the default values for these parameters (if new values are provided through command line arguments they would be used instead of the default ones)
num_of_ss: int = 10     # Number of storage servers available
num_of_dir: int = 3     # Number of dir in each test_dir
max_num_of_files_in_dir: int = 2   # Number of files in each dir
total_num_of_files: int = 5 # Total number of test files in the SS

# Flag to check whether we need to compile all the make files or clean them
clear: int = 0

# If any of these arguments are not provided as command line arguments then the above values will be taken as default
if len(sys.argv) >= 2:
    num_of_ss = int(sys.argv[1])

if len(sys.argv) >= 3:
    num_of_dir = sys.argv[2]
    # If clean is passed as the second argument then cleans all the compiled files
    if (num_of_dir == "clean"):
        clear = 1
        subprocess.run(['rm', 'ss_config.txt'], check = True)
    else:
        num_of_dir = int(num_of_dir)

if len(sys.argv) >= 4:
    max_num_of_files_in_dir = int(sys.argv[3])
    
if len(sys.argv) >= 5:
    total_num_of_files = int(sys.argv[4])

# Getting the path to current working directory
cwd = os.getcwd()

# Copying the SS directory into SS1, SS2, SS3 ... till SSn
copy_folder_n_times(num_of_ss, cwd)

# Looping through all the storage server folders to create test directories and files
for i in range(num_of_ss):
    # If run_clean is 0 then we run the makefile to compile the code
    if clear == 0:
        
        curr_file_index: int = 1
        
        base_dir = os.path.join(cwd, f"SS{i + 1}")
        
        update_header_and_config_files(i, base_dir, cwd)
        
        # Creating multiples directories in the test directory
        for j in range(num_of_dir):
            dir_path = os.path.join(base_dir, f"SS{i + 1}_dir{j + 1}")
            create_dir(dir_path)
            with open(f"{cwd}/SS{i + 1}/accessible_paths.txt", "a") as file:
                file.write(f"./SS{i + 1}_dir{j + 1}")
                file.write(" ")
            
            # Creating sample files with sample text in each dir
            for k in range(max_num_of_files_in_dir):
                file_path = os.path.join(dir_path, f"SS{i + 1}_file{curr_file_index}.txt")
                if curr_file_index <= total_num_of_files:
                    create_file(file_path, curr_file_index)
                    with open(f"{cwd}/SS{i + 1}/accessible_paths.txt", "a") as file:
                        file.write(f"./SS{i + 1}_dir{j + 1}/SS{i + 1}_file{curr_file_index}.txt")
                        file.write(" ")
                    curr_file_index += 1
                
        create_file(base_dir + f"/SS{i + 1}_file{curr_file_index + 1}.txt", curr_file_index + 1)
        create_file(base_dir + f"/SS{i + 1}_file{curr_file_index + 2}.txt", curr_file_index + 2)
        with open(f"{cwd}/SS{i + 1}/accessible_paths.txt", "a") as file:
                file.write(f"./SS{i + 1}_file{curr_file_index + 1}.txt")
                file.write(" ")
                file.write(f"./SS{i + 1}_file{curr_file_index + 2}.txt")
        
        # Compiling all the makefiles
        compile_make(i)
        
    else:
        run_clean(i)

