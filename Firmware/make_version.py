import datetime


def write_version_to_file(filepath, contents):
    with open(filepath, "w", encoding="utf-8") as f:
        f.write(contents)

def make_version_string():
    now = datetime.datetime.now()
    year = now.year
    month = now.month
    day = now.day

    version_header = \
f"""#ifndef __OPENFAN_MICRO_FW_VERSION_H_INC__
#define __OPENFAN_MICRO_FW_VERSION_H_INC__

#define VERSION_MAJOR   {year}
#define VERSION_MINOR   {month}
#define VERSION_PATCH   {day}

#endif
"""

    print(f"New version {year}-{month}-{day}")
    return version_header

def make_version():
    print("--- Cutting version ---")
    version = make_version_string()
    write_version_to_file('src/version.h', version)

Import("env")
make_version()
