import sys
import json

def read_missions(path):
	with open(path, "r", encoding="utf-8") as f:
		lines = [line.rstrip("\n") for line in f]

	i = 0
	n = len(lines)
	missions = []

	while i < n:
		if not lines[i].strip():
			i += 1
			continue

		m = {}

		# ---- HEADER ----
		m["name"] = lines[i]; i += 1
		m["type"] = lines[i]; i += 1
		m["caller"] = lines[i]; i += 1
		m["description"] = lines[i]; i += 1

		# ---- REQUIREMENTS ----
		req_count = int(lines[i]); i += 1
		requirements = []
		for _ in range(req_count):
			requirements.append(lines[i])
			i += 1
		m["requirements"] = requirements

		# ---- POSITION ----
		px, py = map(int, lines[i].split()); i += 1
		m["px"] = px
		m["py"] = py

		# ---- ATTRIBUTES ----
		attr = list(map(int, lines[i].split())); i += 1
		m["attributes"] = {
			"COMBAT": attr[0],
			"VIGOR": attr[1],
			"MOBILITY": attr[2],
			"CHARISMA": attr[3],
			"INTELLIGENCE": attr[4],
		}

		# ---- SLOTS + DIFFICULTY ----
		m["slots"] = int(lines[i]); i += 1
		m["difficulty"] = int(lines[i]); i += 1

		# ---- FAILURE / SUCCESS ----
		fail_dur = int(lines[i]); i += 1
		succ_dur = int(lines[i]); i += 1

		danger = int(lines[i]); i += 1

		m["failure"] = {
			"duration": fail_dur,
			# "message": "",
			# "mission": "",
		}

		m["success"] = {
			"duration": succ_dur,
			# "message": "",
			# "mission": "",
		}

		m["dangerous"] = bool(danger)

		missions.append(m)

	return missions



# ---------------- JSONish Printer ----------------

def jsonish_escape(s):
	return s.replace('"', '\\"')

def to_jsonish(m):
	out = []
	out.append("\t{")
	out.append(f'\t\tname: "{jsonish_escape(m["name"])}",')
	out.append(f'\t\ttype: "{jsonish_escape(m["type"])}",')
	out.append(f'\t\tcaller: "{jsonish_escape(m["caller"])}",')
	out.append(f'\t\tdescription: "{jsonish_escape(m["description"])}",')

	# requirements
	out.append("\t\trequirements: [")
	for r in m["requirements"]:
		out.append(f'\t\t\t"{jsonish_escape(r)}",')
	out.append("\t\t],")

	# attributes
	out.append("\t\tattributes: {")
	for k, v in m["attributes"].items():
		out.append(f"\t\t\t{k}: {v},")
	out.append("\t\t},")

	out.append(f"\t\tposition: [{m["px"]}, {m["py"]}],")
	out.append(f"\t\tslots: {m['slots']},")
	out.append(f"\t\tdifficulty: {m['difficulty']},")

	out.append("\t\tfailure: {")
	out.append(f'\t\t\tduration: {m["failure"]["duration"]},')
	# out.append(f'\t\t\tmessage: "{m["failure"]["message"]}",')
	# out.append(f'\t\t\tmission: "{m["failure"]["mission"]}",')
	out.append("\t\t},")

	out.append("\t\tsuccess: {")
	out.append(f'\t\t\tduration: {m["success"]["duration"]},')
	# out.append(f'\t\t\tmessage: "{m["success"]["message"]}",')
	# out.append(f'\t\t\tmission: "{m["success"]["mission"]}",')
	out.append("\t\t},")

	out.append(f'\t\tdangerous: {"true" if m["dangerous"] else "false"},')
	out.append("\t},")
	return "\n".join(out)



# ---------------- MAIN ----------------

if __name__ == "__main__":
	if len(sys.argv) < 3:
		print("Usage: python convert_missions.py input.txt output.jsonish")
		sys.exit(1)

	input_path = sys.argv[1]
	output_path = sys.argv[2]

	missions = read_missions(input_path)

	with open(output_path, "w", encoding="utf-8") as out:
		out.write("[\n")
		for m in missions:
			out.write(to_jsonish(m))
			out.write("\n")  # spacing
		out.write("]")

	print(f"Converted {len(missions)} missions → {output_path}")
