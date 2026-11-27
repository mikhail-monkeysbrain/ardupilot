#!/usr/bin/env python3
import argparse
import json
from pathlib import Path
from vtx_table_normalize import normalize_cli, normalize_json

PROTOCOLS = {"sa1": 1, "sa2": 2, "sa21": 3, "tramp": 4}
ALPHABET = "\x000123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-"


def encode_label(label):
    if not 1 <= len(label) <= 4 or any(c not in ALPHABET[1:] for c in label):
        raise ValueError(f"unsupported power label: {label!r}")
    return sum(ALPHABET.index(c) << (6*i) for i,c in enumerate(label))


def decode_label(value):
    return "".join(ALPHABET[(value >> (6*i)) & 63] for i in range(4)).rstrip("\x00")


def power_parameters(table, protocol, index=0):
    kind = PROTOCOLS[protocol]
    powers = table["powerlevels"]
    count = len(powers)
    if not 1 <= count <= 10 or not 0 <= index < count:
        raise ValueError("invalid power count or selected index")
    limit = {1:255, 2:255, 3:127, 4:65535}[kind]
    codes = [p["value"] for p in powers]
    if len(set(codes)) != count or any(not 0 <= c <= limit for c in codes):
        raise ValueError("duplicate or invalid protocol power code")
    labels = [encode_label(p["label"]) for p in powers]
    codes += [0] * (10-count)
    labels += [0] * (10-count)
    data = bytearray([kind,count])
    for code,label in zip(codes,labels):
        data.extend(code.to_bytes(2,"little"))
        data.extend(label.to_bytes(3,"little"))
    h = 2166136261
    for b in data:
        h = ((h ^ b) * 16777619) & 0xffffffff
    crc = (h & 0xffffff) | 1
    rows = [("VTX_PCNT",count),("VTX_PTYPE",kind),("VTX_PIDX",index)]
    rows += [(f"VTX_PC{i+1}",v) for i,v in enumerate(codes)]
    rows += [(f"VTX_PL{i+1}",v) for i,v in enumerate(labels)]
    return rows,crc


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source",type=Path)
    parser.add_argument("--protocol",choices=PROTOCOLS,required=True)
    parser.add_argument("--index",type=int,default=0)
    parser.add_argument("-o","--output",type=Path,required=True)
    args = parser.parse_args()
    raw = args.source.read_text(encoding="utf-8-sig")
    table = normalize_json(json.loads(raw)) if args.source.suffix.lower()==".json" else normalize_cli(raw)
    rows,crc = power_parameters(table,args.protocol,args.index)
    invalidate = args.output.with_name(args.output.stem+"_invalidate.param")
    commit = args.output.with_name(args.output.stem+"_commit.param")
    invalidate.write_text("VTX_PCRC,0\n")
    args.output.write_text("".join(f"{k},{v}\n" for k,v in rows))
    commit.write_text(f"VTX_PCRC,{crc}\n")
    print(f"1. {invalidate}\n2. {args.output}\n3. {commit}")
    print("Offline export only. Verify parameters before reboot.")

if __name__ == "__main__":
    main()
