# Introduction 

This is the repository of the eMule Morph project. Note that this repository is a merge of the original CSV repository and the Git repository which was used between MorphXT v11.3 and v12.6. While it contains all history, the history of individual files breaks at the point when the original CSV was left behind for Git.

This repository is not directly linked to the [official Git](https://github.com/SomeSupport/eMule) repository or the [community fork](https://github.com/irwir/eMule) that followed up on its development. Merges from either are made manually.

# How to compile the solution from Git

The project is setup to use Git submodules to include other required libraries. After cloning the repository additional commands must be issued to get the submodules. These are:

```
git submodule init
git submodule update
```

Afterwards, everything is just about ready for compiling using the emule.sln solution file.