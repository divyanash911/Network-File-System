import subprocess
import sys
import os

# Change the operating system name on which device you are running this code (macos/linux)
operating_system = "macos"

# Number of Storage servers i want to start (default = 1), input provided through command line
num_of_ss: int = 1

# Provide the number of SS to start through command line otherwise the default value is 1
if len(sys.argv) >= 2:
    num_of_ss = int(sys.argv[1])

# Getting the path to current working directory
cwd = os.getcwd()

if operating_system == "macos":
    for i in range(num_of_ss):
        os.chdir(f"SS{i + 1}")                  # Changing cwd to the SS folder
        paths = list()
        with open("accessible_paths.txt", "r") as file:
            paths = file.readlines()
        path_string = ""
        for path in paths:
            path_string += path.strip()
            path_string += " "
        command = f'open -a Terminal.app ss.o {path_string}'  # Running the command to open new terminal window and run the .o file
        subprocess.Popen(command, shell=True)
        os.chdir("..")                          # Going back to parent directory

elif operating_system == "linux":
    for i in range(num_of_ss):
        os.chdir(f"SS{i + 1}")                  # Changing cwd to the SS folder 
        paths = list()
        with open("accessible_paths.txt", "r") as file:
            paths = file.readlines()
        path_string = ""
        for path in paths:
            path_string += path.strip()
            path_string += " "
        # Running the command to open new terminal window and run the .o file
        command = f'gnome-terminal -x bash -c "./ss.o {path_string}"'
        subprocess.Popen(command, shell=True)
        os.chdir("..")                          # Going back to parent directory

    


