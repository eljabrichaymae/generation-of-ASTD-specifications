import json
import sys
from typing import Dict, Any

def remove_invalid_utf8(text: str) -> str:
    return text.encode('utf-8', errors='replace').decode('utf-8')

def extract_function_name(data: Dict[str, Any]) -> str:
    keys_to_try = ['FunctionName', 'Functionname', 'functionName', 'function_name', 'name']
    for key in keys_to_try:
        if key in data:
            return remove_invalid_utf8(str(data[key]))
    return "N/A"

def process_line(line: str, line_number: int) -> str:
    try:
        line_clean = remove_invalid_utf8(line.strip())
        if not line_clean:
            return None
        data = json.loads(line_clean)
        return extract_function_name(data)
    except json.JSONDecodeError as e:
        print(f"[Ligne {line_number}] Erreur JSON - ligne ignorée: {e.msg}", file=sys.stderr)
        return None
    except Exception as e:
        print(f"[Ligne {line_number}] Erreur inattendue: {str(e)}", file=sys.stderr)
        return None

def main():
    if len(sys.argv) != 2:
        print("Usage: python extractFunctionnames.py <input_file>", file=sys.stderr)
        sys.exit(1)

    input_file = sys.argv[1]
    success_count = 0
    error_count = 0

    try:
        with open(input_file, 'r', encoding='utf-8', errors='replace') as infile, \
             open("function_names.txt", 'w', encoding='utf-8') as outfile:

            for line_number, line in enumerate(infile, 1):
                function_name = process_line(line, line_number)
                if function_name is not None:
                    outfile.write(f"e({function_name})\n")
                    success_count += 1
                else:
                    error_count += 1

            outfile.write("end")
            
        #print(f"Succès: {success_count}, Erreurs: {error_count}")
    except Exception as e:
        print(f"Erreur critique: {str(e)}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()