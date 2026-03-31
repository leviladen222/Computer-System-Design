#!/usr/bin/env python3

from functools import reduce
import argparse, json, logging, os, sys, toml

def argparser():
    parser = argparse.ArgumentParser(
        description="""
        Checks the message *bodies* from a response dir by replaying a workload.
        """
    )
    parser.add_argument(
        "-r", "--replay-dir", type=str, default="replay",
        help="directory to replay responses in"
    )
    parser.add_argument(
        "-d", "--response-dir", type=str, default="responses",
        help="directory where responses where stored"
    )
    parser.add_argument(
        "workload", type=argparse.FileType("r"), metavar="workload",
        help="workload that we're replaying (and testing)"
    )
    return parser.parse_args()

class Event:
    def __init__(self, fields):
        self.fields = fields

    def __str__(self):
        return f"[[events]]\n{toml.dumps(self.fields)}".strip()

    def __repr__(self):
        return f"Event {json.dumps(self, default=lambda x: vars(x), indent=4)}"

    def replay(self, replay_dir):
        pass

class Load(Event):
    def replay(self, replay_dir):
        infile = self.fields["infile"]
        outfile = self.fields["outfile"]
        os.system(f"cp {infile} {outfile}")
        logging.info(f"LOAD: {infile} to {outfile}")

class Unload(Event):
    def replay(self, replay_dir):
        file = self.fields["file"]
        os.system(f"rm -f {file}")
        logging.info(f"UNLOAD: {file}")

class Request(Event):
    def replay(self, replay_dir):
        rid = self.fields["id"]
        uri = self.fields["uri"]

        response = os.path.join(replay_dir, f"{rid}-out")
        if not os.path.exists(uri):
            with open(response, "w") as f:
                print("Not Found", flush=True, file=f)
        else:
            os.system(f"cp {uri} {response}")
        logging.info(f"Request: {uri}")

def identify_event(fields) -> Event: 
    event = None
   
    return event

def parse_events_toml(infile):
    for fields in toml.load(infile)["events"]:
        if fields["type"] == "LOAD":
            yield Load(fields)
        elif fields["type"] == "UNLOAD":
            yield Unload(fields)
        elif fields["type"] == "REQUEST":
            yield Request(fields)
        else:
            assert (False)

def validate_responses(replay_dir, response_dir):
    status = 0
    if os.system(f"diff -r {replay_dir} {response_dir} > /dev/null 2>&1") != 0:
        print("[Coroner] output files differ")
        status = 1
    return status

def main():
    args = argparser()
    events = parse_events_toml(args.workload)

    if len(args.replay_dir) > 0:
        try:
            os.mkdir(args.replay_dir)
        except:
            pass
        
    for event in events: 
        event.replay(args.replay_dir)
    
    status = validate_responses(args.replay_dir, args.response_dir)
    sys.exit(status)

if __name__ == "__main__":
    main()
