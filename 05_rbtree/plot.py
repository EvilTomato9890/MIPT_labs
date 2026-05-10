from pathlib import Path
import sys

sys.path.append(str(Path(__file__).resolve().parent.parent / "common"))
from plot_tree import plot_tree

plot_tree(Path(__file__).resolve().parent, "Red-Black")
