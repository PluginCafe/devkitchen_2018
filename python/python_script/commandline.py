# Retrieves a command-line argument
# Note argparse module obtains the command-line arguments from sys.argv
# VS Code command-line arguments: "args": ["-myArgument=10"]

import argparse

def main():
    # Creates an ArgumentParser
    parser = argparse.ArgumentParser()
    # Adds 'myArgument' required integer argument
    parser.add_argument('-myArgument', type=int, required=True, help='My integer argument')
    # Parses the arguments passed to the script
    args = parser.parse_args()
    # Retrieves and prints the value for 'myArgument'
    print('myArgument value: {}'.format(args.myArgument))


if __name__=='__main__':
    main()
