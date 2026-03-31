#!/usr/bin/env python3

import argparse, os, toml, time, sys, shutil, re, socket

class Logger(): 
    def __init__(self):
        self.events = []

    def log(self, event, *argv):
        stamp = time.time()
        fields = [stamp, event]
        if argv:
            fields.extend(argv)
        self.events.append(fields)

    def flush(self):
        for l in self.events:
            print(", ".join([str(f) for f in l]))
        self.events.clear()

class Host():
    hosts = {}
    def __new__(cls, id: int, hostname: str, port: int):
        raise RuntimeError("Use Host.add_host() instead of direct instantiation.")
    
    @classmethod
    def _create(cls,  id: int, hostname: str, port: int):
        instance = super().__new__(cls)
        instance.id = id
        instance.hostname = hostname
        instance.port = port
        return instance

    @classmethod
    def add_host(cls, id: int, hostname: str, port: int):
        Host.hosts[id] = Host._create(id, hostname, port)

    def get_host(id: int): 
        return Host.hosts[id]

    def __repr__(self): 
        return f"({self.id}, {self.hostname}:{self.port})"

    def get_hostname(self) -> str:
        return self.hostname

    def get_port(self) -> str:
        return self.port
    
    def get_id(self) -> int:
        return self.id
    

class Proxy_Request():
    '''A request to the proxy'''

    RESPONSE_HEADER_REGEX = re.compile(b"HTTP/1.1 ([^ ]*) (.*)\r\n")
    CACHED_HEADER_STRING = b"Cached: True\r\n"
    BLOCK = 4096

    def __init__(self, host: Host, id: int, uri: str):
        request= f"GET http://{host.get_hostname()}:{host.get_port()}/{uri} HTTP/1.1\r\n\r\n"

        self.id = id
        self.uri = uri
        self.bytez = bytearray(request.encode('UTF-8'))
        self.code = -1
        self.cached = False
        self.hostid = host.get_id()

    def __repr__(self):
        return f"request:{self.bytez}"

    def doit(self, proxyhost: str, proxyport: str, outfile: str) -> bool:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((socket.gethostbyname(proxyhost), proxyport))
        sock.sendall(self.bytez)
        received = bytearray()

        data = sock.recv(Proxy_Request.BLOCK)
        while data:
            received.extend(data)
            data = sock.recv(Proxy_Request.BLOCK)

        # parse the response to get the code:
        match = re.match(Proxy_Request.RESPONSE_HEADER_REGEX, received)
        if match:
            self.code = int(match.group(1))
        
        # parse the response to get cached:
        self.cached = (received.find(Proxy_Request.CACHED_HEADER_STRING) > 0)
        bodyindex = received.find(b"\r\n\r\n")

        if self.code > 0 and bodyindex > 0:
            open(outfile, "wb+").write(received[bodyindex + 4:])
        #print(f"[Rosemaylie] match={match}, bodyindex={bodyindex}, cahed={self.cached}, man={received.find(Proxy_Request.CACHED_HEADER_STRING)}, body={received}", file=sys.stderr)
        return self.code > 0 and bodyindex > 0

    def get_uri(self) -> str:
        return self.uri
    
    def get_id(self) -> int:
        return self.id
    
    def get_code(self) -> int:
        return self.code
    
    def get_cached(self) -> bool:
        return self.cached
    
    def get_hostid(self) -> int:
        return self.hostid
    

def argparser():
    parser = argparse.ArgumentParser(description="Issues a batch of requests.")
    parser.add_argument(
        "-o", "--proxyhost", type=str, default="localhost",
        metavar="localhost", help="Hostname to connect to for the proxy."
    )
    parser.add_argument(
        "-p", "--proxyport", type=int,
        metavar="port", help="Port to connect to for the proxy."
    )

    parser.add_argument(
        "-d", "--dir", type=str, default="",
        metavar="dir", help="Where to place the message bodies."
    )

    parser.add_argument(
        "requests", type=str, metavar="requests",
        help="file containing a batch of tests."
    )
    return parser.parse_args()

def host(event):
    if "hostname" in event and "port" in event and "id" in event:
        Host.add_host(int(event["id"]), event["hostname"], int(event["port"]))
    else:
        print(f"invalid host: {event}", file=sys.stderr)


def load(event):
    if "infile" in event and "outfile" in event:
        try:
            shutil.copyfile(event["infile"], event["outfile"])
        except FileExistsError:
            os.unlink(event["outfile"])
            shutil.copyfile(event["infile"], event["outfile"])
    else:
        print(f"invalid load, missing infile or outfile: {event}", file=sys.stderr)

def unload(event):
    if "file" in event:
        try:
            os.remove(event["file"])
        except:
            pass
    else:
        print(f"invalid unload, missing infile or outfile: {event}", file=sys.stderr)

def create(event:toml) -> Proxy_Request:
    if "uri" not in event or "host" not in event or "id" not in event:
        print(f"Invalid request, {event}, ", file=sys.stderr)
        return None
    else:
        uri = event["uri"]
        host = int(event["host"])
        id = int(event["id"])
        return Proxy_Request(Host.get_host(host), id, uri)

def main():
    logger = Logger()
    args = argparser()
    
    proxyhost = args.proxyhost
    proxyport = args.proxyport
    requests = args.requests
    outdir = args.dir
    ret = 0
    
    if len(outdir) > 0:
        try:
            os.mkdir(outdir)
        except:
            pass

    for event in toml.load(requests)["events"]:
        rid = -1
        if "id" in event:
            rid = event["id"]

        if event["type"] == "LOAD":
            load(event)
            logger.log("LOAD", f'{event["infile"]}, {event["outfile"]}')

        elif event["type"] == "UNLOAD":
            unload(event)
            logger.log("UNLOAD", f'{event["file"]}')

        elif event["type"] == "HOST":
            host(event)
            ## no need to log.

        elif event["type"] == "REQUEST":
            # Create the request:
            prequest = create(event)

            ## Get next id for this request
            outfile = f"{prequest.get_id()}-out"
            if len(outdir) > 0:
                outfile = f"{outdir}/{outfile}"

            ## Do the request (_doit_) then log.
            if not prequest.doit(proxyhost, proxyport, outfile):
                print(f"[Rosemaylie] response not parsed for {prequest.get_id()}", file=sys.stderr)
                ret = 1
            logger.log("REQUEST", f"{prequest.get_hostid()}", f"{prequest.get_uri()}" , f"{prequest.get_code()}", f"{prequest.get_cached()}")

        else:
            print(f"That event, Rosie, is an imposter: {event}")

    logger.flush()
    return ret

if __name__ == "__main__":
    sys.exit(main())
