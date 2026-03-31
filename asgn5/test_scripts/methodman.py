#!/usr/bin/env python3

from functools import reduce
import argparse, sys, os


MB = (1<<20)

class cacheSimulator:
    def __init__(self, cache_size, cache_alg):
        self.cache_size = cache_size
        self.cache_alg = cache_alg
        self.cache = []
        self.cache_set = set() 

    def fifo(self, req):
        if req in self.cache_set:
            return True
        
        if len(self.cache) >= self.cache_size:
            evicted = self.cache.pop(0)        # expensive but whatever.
            self.cache_set.remove(evicted)

        self.cache.append(req)
        self.cache_set.add(req)
        return False

    def lru(self, req): 
        if req in self.cache_set:
            self.cache.remove(req)  # expensive, but whatever
            self.cache.append(req)
            return True

        if len(self.cache) >= self.cache_size:
            evicted = self.cache.pop(0)
            self.cache_set.remove(evicted)

        self.cache.append(req)
        self.cache_set.add(req)
        return False

    def access_cache(self, host: str, uri: str):
        req = f"{host}/{uri}"
        if self.cache_alg == "FIFO":
            return self.fifo(req)
        
        if self.cache_alg == "LRU":
            return self.lru(req)
        
def process_log(logfile, cache_simulator):
    total_req = 0
    mismatches = 0
    cacheable = {}
    events = [line for line in logfile]

    loads = [line for line in events if "LOAD" in line and "UNLOAD" not in line]
    for line in loads:
        (_, _, infile, outfile) = line.strip().split(", ")
        stat = os.stat(infile)
        if stat.st_size < MB:
            cacheable[outfile] = True

    requests = [line for line in events if "REQUEST" in line]
    for count, line in enumerate(requests):
        (_, _, host, uri, _, cached) = line.strip().split(", ")
        observed = cached.lower() == "true"

        sim_cached = False
        if uri in cacheable:
            sim_cached = cache_simulator.access_cache(host, uri)

        if sim_cached != observed:
            print(f"[Methodman] MISMATCH on request {count}, Expected: {sim_cached}, found {observed}")
            mismatches += 1
        
        total_req += 1

    print(f"\n[Summary] Total Requests: {total_req}, Mismatches: {mismatches}")
    if mismatches > 0:
        print("[Methodman] Oh no! Rose log is different from cache simulation.")
    return mismatches == 0

def argparser():
    parser = argparse.ArgumentParser(
        description="""
        Checks the message *bodies* from a response dir by replaying a workload.
        """
    )
    parser.add_argument(
        "-a", "--cache-alg", choices=["FIFO", "LRU"], default="replay",
        help="The cache algorithm to simulate"
    )
    parser.add_argument(
        "-s", "--cache-size", type=int, default="responses",
        help="the size of the cache to simualte"
    )
    parser.add_argument(
        "roselog", type=argparse.FileType("r"), metavar="ROSELOG",
        help="The log that rose reported for cached entries"
    )
    return parser.parse_args()

def main():
    args = argparser()

    cache_simulator = cacheSimulator(args.cache_size, args.cache_alg)
    success = process_log(args.roselog, cache_simulator)
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
