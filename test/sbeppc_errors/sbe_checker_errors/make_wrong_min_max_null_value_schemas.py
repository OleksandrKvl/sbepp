import os

# Define parameters for file generation
kinds = ["min", "max", "null"]
types = ["int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64", "float", "double"]

# Out-of-range values for each type
out_of_range_values = {
    "int8": "128",
    "int16": "32768",
    "int32": "2147483648",
    "int64": "9223372036854775808",
    "uint8": "-1",
    "uint16": "-1",
    "uint32": "-1",
    "uint64": "-1",
    "float": "3.4028235e+39",
    "double": "1.7976931348623157e+309"
}

# Directory to save files
output_dir = "xml_files"
os.makedirs(output_dir, exist_ok=True)

# XML template
xml_template = """<?xml version="1.0" encoding="UTF-8"?>
<sbe:messageSchema xmlns:sbe="http://fixprotocol.io/2016/sbe"
                   xmlns:xi="http://www.w3.org/2001/XInclude"
                   package="test_schema"
                   id="1"
                   version="0"
                   semanticVersion="5.2"
                   description="Example base schema which can be extended."
                   byteOrder="littleEndian">
    <types>
        <type name="bad_type" primitiveType="{type}" presence="optional" {kind}Value="{value}"/>
    </types>
</sbe:messageSchema>
"""

# Generate files
for kind in kinds:
    for type_name in types:
        file_name = f"wrong_{kind}_value_{type_name}.xml"
        file_path = os.path.join(output_dir, file_name)
        value = out_of_range_values[type_name]

        # Populate XML content
        xml_content = xml_template.format(type=type_name, kind=kind, value=value)

        # Write the XML file
        with open(file_path, "w", encoding="utf-8") as file:
            file.write(xml_content)

print(f"XML files generated in '{output_dir}' directory.")
