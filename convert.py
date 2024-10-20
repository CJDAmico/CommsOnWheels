import json
import re

def parse_dbc_file(dbc_file_path):
    db_dict = {
        'buses': [],
        'messages': [],
        'nodes': []
    }
    
    with open(dbc_file_path, 'r') as file:
        current_message = None
        
        for line in file:
            line = line.strip()
            if not line or line.startswith('CM'):  # Skip comments and empty lines
                continue

            # Match bus definition
            if line.startswith('BU_'):
                parts = line.split()
                for bus in parts[1:]:  # Bus names are after the first element
                    db_dict['buses'].append({'name': bus, 'baud': '250k'})  # Default baud, update as necessary

            # Match message definition
            elif line.startswith('BO_'):
                parts = line.split()
                if len(parts) < 4:  # Ensure there are enough parts
                    print(f"Skipping invalid BO_ line: {line}")
                    continue
                
                if current_message:
                    db_dict['messages'].append(current_message)
                
                try:
                    frame_id = int(parts[1])
                    name = parts[2]
                    length = int(parts[3])
                except (ValueError, IndexError) as e:
                    print(f"Error parsing BO_ line: {line}, Error: {e}")
                    continue
                
                # Initialize a new message structure
                current_message = {
                    'pgn': frame_id,
                    'name': name,
                    'description': '',  # Add description if available in the DBC file
                    'priority': 3,  # Default value; update based on DBC if available
                    'length': length,
                    'tx_periodicity': 0,  # Update based on DBC info if needed
                    'tx_onChange': False,  # Update based on DBC info if needed
                    'data': []
                }

            # Match signal definition
            elif line.startswith('SG_') and current_message is not None:
                parts = line.split(' : ')
                if len(parts) < 2:  # Ensure there's a signal name and definition
                    print(f"Skipping invalid SG_ line: {line}")
                    continue
                
                signal_name = parts[0].split()[1]  # Extract signal name from the left part
                signal_definition = parts[1]  # The right part contains the actual signal definition

                # Use regular expression to extract the different components
                match = re.match(r'(\d+)\|(\d+)@(\d)([+-]?) \(([^,]+),([0-9-]+)\) \[(-?\d+)\|(\d+)\] "(.*?)" (.+)', signal_definition)
                if not match:
                    print(f"Error parsing SG_ line: {line}, unable to match pattern")
                    continue

                start_bit = int(match.group(1))
                signal_length = int(match.group(2))
                endianness = match.group(3)  # 0 for little-endian, 1 for big-endian
                is_bigEndian = endianness == '1'
                is_twosComplement = match.group(4) == '-'
                factor = float(match.group(5))
                offset = float(match.group(6))
                min_value = int(match.group(7))
                max_value = int(match.group(8))
                units = match.group(9).strip('"')  # Remove surrounding quotes
                node_name = match.group(10)

                # Add the signal to the current message's data
                current_message['data'].append({
                    'spn': 0,  # Use 0 as default, update based on DBC if SPN is available
                    'name': signal_name,
                    'description': '',  # Add signal description if available
                    'start_bit': start_bit,
                    'bit_length': signal_length,
                    'is_bigEndian': is_bigEndian,
                    'is_twosComplement': is_twosComplement,
                    'factor': factor,
                    'offset': offset,
                    'units': units,
                    'scaled_min': min_value,
                    'scaled_max': max_value,
                    'scaled_default': None,
                    'enumerations': []  # Add enumeration values if available in the DBC
                })
        
        # Append the last message to the messages list if it exists
        if current_message:
            db_dict['messages'].append(current_message)

    # Dynamically create nodes based on the buses and messages
    for bus in db_dict['buses']:
        node = {
            'name': 'EngineECU',  # Replace with dynamic name if needed
            'buses': [
                {
                    'name': bus['name'],
                    'source_address': 0,  # Set the source address, valid values are 0-255
                    'tx': [{'name': msg['name']} for msg in db_dict['messages']],
                    'rx': [{'name': 'TC1'}]  # Example receive message, modify based on DBC data
                }
            ]
        }
        db_dict['nodes'].append(node)
    
    return db_dict

# Path to the DBC file and output JSON file
dbc_file_path = 'Sample Files\\J1939 DBC\\weird.dbc'  # Update with your file path
json_file_path = 'Sample Files\\JSON\\weird.json'  # Update with your desired output path

# Parse the DBC file and convert to JSON
db_dict = parse_dbc_file(dbc_file_path)
with open(json_file_path, 'w') as json_file:
    json.dump(db_dict, json_file, indent=4)

print(f"J1939 DBC file converted to JSON and saved at: {json_file_path}")