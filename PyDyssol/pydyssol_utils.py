# dyssol_utils.py
def pretty_print(data):
    units = {"mass": "kg", "massflow": "kg/s", "temperature": "K", "pressure": "Pa"}
    print("=== Overall ===")
    for k, v in data["overall"].items():
        unit = units.get(k, "")
        if isinstance(v, (tuple, list)):
            val, unit = v
        else:
            val = v
        print(f"{k:<25}: {val:.4f} {unit}")
    print("\n=== Composition ===")
    for k, v in data["composition"].items():
        print(f"{k:<25}: {v:.4f} kg")
    print("\n=== Distributions ===")
    for name, dist in data["distributions"].items():
        print(f"\n{name}:\n" + "\n".join(f"{x:.4e}" for x in dist))
