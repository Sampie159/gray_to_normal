A grayscale to normal map converter.

# Usage

Simply run

```bash
gtn <file_name> ...
```

You can also run

```bash
gtn -h
```

for a help menu.

# Build From Source

Simply run

```bash
make
```

to build the executable `gtn`.

## Installation

You can also run

```bash
sudo make install
```

to install the executable on your system.

And also

```bash
sudo make uninstall
```

to remove the executable from your system.

# Dependencies

A compiler that supports `-std=gnu++26`.

# Nix

You can try this program with Nix by simply calling
```bash
nix shell github:Sampie159/gray_to_normal
```
