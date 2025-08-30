import socket

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

    
print(get_local_ip())