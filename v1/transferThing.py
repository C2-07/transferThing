#!/usr/bin/env python3

import socket
import threading
import os
import sys
import json
import struct
import time

# --- Configuration ---
# Use a non-privileged port.
PORT = 65432
# Number of parallel connections to use.
NUM_THREADS = 8
# Buffer size for sending/receiving data. Increased for potentially better performance.
BUFFER_SIZE = 262144*4 # 1MB buffer

# --- Style Constants ---
COLOR_BLUE = '\033[94m'
COLOR_GREEN = '\033[92m'
COLOR_YELLOW = '\033[93m'
COLOR_RED = '\033[91m'
COLOR_BOLD = '\033[1m'
COLOR_END = '\033[0m'

def get_local_ip():
    """
    Finds the local IP address of the machine.
    Connects to a public DNS server to find the primary network interface.
    """
    s = None
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # Doesn't have to be reachable
        s.connect(('8.8.8.8', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        if s:
            s.close()
    return IP

def print_progress_bar(progress_list, total_size, start_time, lock):
    """
    Displays or updates a console progress bar.
    Accepts a list of progress for each thread.
    """
    # Small delay to prevent the bar from flashing before the transfer starts
    time.sleep(0.1)
    while sum(progress_list) < total_size:
        with lock:
            current_bytes = sum(progress_list)
        
        if total_size == 0:
            percentage = 100
        else:
            percentage = (current_bytes / total_size) * 100
        
        bar_length = 50
        # Handle the case where current_bytes might exceed total_size slightly due to threading
        filled_length = int(bar_length * min(current_bytes, total_size) // total_size)
        bar = '█' * filled_length + '-' * (bar_length - filled_length)

        elapsed_time = time.time() - start_time
        speed = (current_bytes / (1024 * 1024)) / elapsed_time if elapsed_time > 0 else 0
        
        mb_sent = current_bytes / (1024 * 1024)
        mb_total = total_size / (1024 * 1024)

        sys.stdout.write(
            f'\r{COLOR_GREEN}Progress: |{bar}| {percentage:.2f}% '
            f'({mb_sent:.2f}/{mb_total:.2f} MB) @ {speed:.2f} MB/s{COLOR_END}'
        )
        sys.stdout.flush()
        time.sleep(0.1)


    # Final print to show 100% completion
    sys.stdout.write(
        f'\r{COLOR_GREEN}Progress: |{"█" * 50}| 100.00% '
        f'({total_size / (1024*1024):.2f}/{total_size / (1024*1024):.2f} MB) - Done!{COLOR_END}\n'
    )
    sys.stdout.flush()


# ==============================================================================
# SENDER (SERVER) LOGIC
# ==============================================================================

def send_chunk(conn, filepath, start_byte, end_byte, progress_list, thread_id, lock):
    """
    Sends a specific chunk of a file over a socket connection.
    """
    try:
        with open(filepath, 'rb') as f:
            f.seek(start_byte)
            bytes_to_send = end_byte - start_byte
            bytes_sent = 0
            while bytes_sent < bytes_to_send:
                chunk_size = min(BUFFER_SIZE, bytes_to_send - bytes_sent)
                data = f.read(chunk_size)
                if not data:
                    break # Should not happen if logic is correct
                conn.sendall(data)
                bytes_sent += len(data)
                with lock:
                    progress_list[thread_id] += len(data)
    except Exception as e:
        print(f"\n{COLOR_RED}Error in thread {thread_id}: {e}{COLOR_END}")
    finally:
        try:
            conn.shutdown(socket.SHUT_WR)
        except OSError:
            pass # Socket might already be closed
        conn.close()

def server_logic(filepath):
    """
    Main logic for the sender (server).
    Waits for clients, sends metadata, and dispatches threads to send file chunks.
    """
    if not os.path.exists(filepath):
        print(f"{COLOR_RED}Error: File not found at '{filepath}'{COLOR_END}")
        return

    filesize = os.path.getsize(filepath)
    filename = os.path.basename(filepath)
    local_ip = get_local_ip()

    print(f"{COLOR_BOLD}--- File Sender ---{COLOR_END}")
    print(f"File: {COLOR_YELLOW}{filename}{COLOR_END} ({filesize / (1024*1024):.2f} MB)")
    
    connections = []
    threads = []
    progress_list = [0] * NUM_THREADS
    lock = threading.Lock()

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind((local_ip, PORT))
        listener.listen(NUM_THREADS)
        print(f"Server listening on {COLOR_BLUE}{local_ip}:{PORT}{COLOR_END}")
        print(f"Waiting for {NUM_THREADS} connections from receiver...")

        # Accept all connections
        for i in range(NUM_THREADS):
            conn, addr = listener.accept()
            connections.append(conn)
            print(f"Accepted connection {i+1}/{NUM_THREADS} from {addr}")

        print(f"\n{COLOR_GREEN}All clients connected. Preparing to send metadata...{COLOR_END}")

        # Define chunks
        chunk_size = filesize // NUM_THREADS
        chunk_ranges = []
        for i in range(NUM_THREADS):
            start = i * chunk_size
            end = (i + 1) * chunk_size if i < NUM_THREADS - 1 else filesize
            chunk_ranges.append((start, end))

        # Prepare and send metadata
        metadata = {
            "filename": filename,
            "filesize": filesize,
            "num_threads": NUM_THREADS,
            "chunks": chunk_ranges
        }
        metadata_bytes = json.dumps(metadata).encode('utf-8')
        payload = struct.pack('>L', len(metadata_bytes)) + metadata_bytes
        
        # Send metadata to the first connection
        connections[0].sendall(payload)

        print("Metadata sent. Starting file transfer...")
        start_time = time.time()

        # Start progress bar thread
        progress_thread = threading.Thread(target=print_progress_bar, args=(progress_list, filesize, start_time, lock))
        progress_thread.start()

        # Start sender threads
        for i in range(NUM_THREADS):
            conn = connections[i]
            start_byte, end_byte = chunk_ranges[i]
            thread = threading.Thread(
                target=send_chunk,
                args=(conn, filepath, start_byte, end_byte, progress_list, i, lock)
            )
            threads.append(thread)
            thread.start()

        # Wait for all threads to complete
        for thread in threads:
            thread.join()
        
        progress_thread.join()

        total_time = time.time() - start_time
        print(f"\n{COLOR_BOLD}{COLOR_GREEN}File transfer complete!{COLOR_END}")
        print(f"Total time: {total_time:.2f} seconds.")


# ==============================================================================
# RECEIVER (CLIENT) LOGIC
# ==============================================================================

def recv_payload(sock):
    """
    Receives a size-prefixed payload from a socket.
    """
    # Read message length
    raw_msglen = sock.recv(4)
    if not raw_msglen:
        return None
    msglen = struct.unpack('>L', raw_msglen)[0]
    
    # Read the message data in chunks
    data = b''
    while len(data) < msglen:
        packet = sock.recv(min(BUFFER_SIZE, msglen - len(data)))
        if not packet:
            return None
        data += packet
    return data

def receive_chunk(sock, part_filename, chunk_size, progress_list, thread_id, lock):
    """
    Receives a file chunk and saves it to a part file.
    """
    try:
        with open(part_filename, 'wb') as f:
            bytes_received = 0
            while bytes_received < chunk_size:
                chunk = sock.recv(min(BUFFER_SIZE, chunk_size - bytes_received))
                if not chunk:
                    # Connection closed prematurely
                    break
                f.write(chunk)
                bytes_received += len(chunk)
                with lock:
                    progress_list[thread_id] += len(chunk)
    except Exception as e:
        print(f"\n{COLOR_RED}Error in thread {thread_id}: {e}{COLOR_END}")
    finally:
        sock.close()

def join_files(base_filename, num_parts):
    """
    Joins the downloaded part files into a single final file.
    """
    print(f"\nAssembling file: {COLOR_YELLOW}{base_filename}{COLOR_END}")
    try:
        with open(base_filename, 'wb') as final_file:
            for i in range(num_parts):
                part_filename = f"{base_filename}.part{i}"
                if not os.path.exists(part_filename):
                    print(f"{COLOR_RED}Error: Missing part file {part_filename}{COLOR_END}")
                    return
                with open(part_filename, 'rb') as part_file:
                    final_file.write(part_file.read())
                os.remove(part_filename) # Clean up part file
        print(f"{COLOR_GREEN}File assembled successfully!{COLOR_END}")
    except Exception as e:
        print(f"{COLOR_RED}Error joining files: {e}{COLOR_END}")

def client_logic(server_ip):
    """
    Main logic for the receiver (client).
    Connects to the server, receives metadata, and downloads chunks in parallel.
    """
    print(f"{COLOR_BOLD}--- File Receiver ---{COLOR_END}")
    
    sockets = []
    threads = []
    lock = threading.Lock()

    try:
        print(f"Connecting to server at {COLOR_BLUE}{server_ip}:{PORT}{COLOR_END}...")
        for i in range(NUM_THREADS):
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((server_ip, PORT))
            sockets.append(s)
            print(f"Established connection {i+1}/{NUM_THREADS}...")

        print(f"\n{COLOR_GREEN}All connections established. Waiting for metadata...{COLOR_END}")

        # Receive metadata from the first socket
        metadata_bytes = recv_payload(sockets[0])
        if not metadata_bytes:
            raise ConnectionError("Did not receive metadata from server.")
        
        metadata = json.loads(metadata_bytes.decode('utf-8'))
        filename = metadata['filename']
        filesize = metadata['filesize']
        chunks = metadata['chunks']
        
        # Handle potential filename collision
        if os.path.exists(filename):
            print(f"{COLOR_YELLOW}Warning: File '{filename}' already exists.{COLOR_END}")
            base, ext = os.path.splitext(filename)
            count = 1
            while os.path.exists(f"{base}_{count}{ext}"):
                count += 1
            filename = f"{base}_{count}{ext}"
            print(f"Saving as '{filename}' instead.")

        print(f"Receiving file: {COLOR_YELLOW}{filename}{COLOR_END} ({filesize / (1024*1024):.2f} MB)")
        
        progress_list = [0] * NUM_THREADS
        start_time = time.time()

        # Start progress bar thread
        progress_thread = threading.Thread(target=print_progress_bar, args=(progress_list, filesize, start_time, lock))
        progress_thread.start()

        # Start receiver threads
        for i in range(NUM_THREADS):
            sock = sockets[i]
            part_filename = f"{filename}.part{i}"
            start_byte, end_byte = chunks[i]
            chunk_size = end_byte - start_byte
            thread = threading.Thread(
                target=receive_chunk,
                args=(sock, part_filename, chunk_size, progress_list, i, lock)
            )
            threads.append(thread)
            thread.start()

        # Wait for all threads to complete
        for thread in threads:
            thread.join()
        
        progress_thread.join()
        
        # Join the downloaded parts
        join_files(filename, NUM_THREADS)

    except (ConnectionRefusedError, TimeoutError):
        print(f"{COLOR_RED}Connection failed. Is the server running at {server_ip}?{COLOR_END}")
    except Exception as e:
        print(f"\n{COLOR_RED}An error occurred: {e}{COLOR_END}")
    finally:
        for s in sockets:
            s.close()


# ==============================================================================
# MAIN EXECUTION
# ==============================================================================

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"{COLOR_BOLD}Usage:{COLOR_END}")
        print(f"  {COLOR_GREEN}To send a file:{COLOR_END} python {sys.argv[0]} /path/to/your/file.ext")
        print(f"  {COLOR_GREEN}To receive a file:{COLOR_END} python {sys.argv[0]} recv <server_ip_address>")
        sys.exit(1)

    mode_or_filepath = sys.argv[1]

    if mode_or_filepath.lower() == 'recv':
        if len(sys.argv) != 3:
            print(f"{COLOR_RED}Error: Server IP address is required for receive mode.{COLOR_END}")
            print(f"  {COLOR_GREEN}Example:{COLOR_END} python {sys.argv[0]} recv 192.168.1.5")
            sys.exit(1)
        server_ip = sys.argv[2]
        client_logic(server_ip)
    else:
        if len(sys.argv) != 2:
            print(f"{COLOR_RED}Error: Invalid arguments for send mode.{COLOR_END}")
            print(f"{COLOR_BOLD}Usage:{COLOR_END}")
            print(f"  {COLOR_GREEN}To send a file:{COLOR_END} python {sys.argv[0]} /path/to/your/file.ext")
            print(f"  {COLOR_GREEN}To receive a file:{COLOR_END} python {sys.argv[0]} recv <server_ip_address>")
            sys.exit(1)
        server_logic(mode_or_filepath)