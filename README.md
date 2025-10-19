# transferThing

A simple command-line tool for transferring files between two computers on the same network.

## How to Use

### Prerequisites

- A C compiler (like GCC or Clang)
- Make

### Building

To build the project, simply run the `make` command in the root directory:

```bash
make
```

This will create an executable named `tt` in the root directory.

### Sending a File

To send a file, use the `send` command followed by the path to the file:

```bash
./tt send <file>
```

For example:

```bash
./tt send my_document.txt
```

The sender will then wait for a receiver to connect.

### Receiving a File

To receive a file, use the `recv` command:

```bash
./tt recv
```

The receiver will search for a sender on the network. Once a sender is found, the file transfer will begin.

## How to Contribute

Contributions are welcome! If you would like to contribute to this project, please follow these steps:

1.  **Fork the repository.**
2.  **Create a new branch for your feature or bug fix.**

    ```bash
    git checkout -b my-new-feature
    ```

3.  **Make your changes.**
4.  **Commit your changes.**

    ```bash
    git commit -am 'Add some feature'
    ```

5.  **Push to the branch.**

    ```bash
    git push origin my-new-feature
    ```

6.  **Create a new Pull Request.**

### Code Style

Please try to follow the existing code style. The project uses a simple and consistent style.

### Reporting Bugs

If you find a bug, please open an issue on the GitHub repository. Please include as much detail as possible, including:

-   A clear and concise description of the bug.
-   Steps to reproduce the bug.
-   The expected behavior.
-   The actual behavior.
-   Your operating system and compiler version.
