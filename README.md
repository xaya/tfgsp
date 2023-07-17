This is the [GSP] (https://github.com/xaya/xaya_tutorials/wiki/The-Game-Processor) for Treat Fighter.
It has been redeveloped from the great work by Tricky Fast but is now using the latest and complete Xaya SDK along with some extra features.
To build you will first need to build [Libxayagame] (https://github.com/xaya/xaya_tutorials/wiki/How-to-Compile-libxayagame-in-Windows)

Then:

```
./autogen.sh
./configure
make
```

On Windows, under Msys2 + Mingw64, it should properly finish all the unit tests:

```
make check
```

All the name_updates submited by wallet are verified and processed by [move processor] (https://github.com/xaya/tfgsp/blob/main/src/moveprocessor.hpp)
Then any ongoing logic is processed inside core [logic class] (https://github.com/xaya/tfgsp/blob/main/src/logic.hpp)
Client fetches the [gamestate](https://github.com/xaya/tfgsp/blob/main/src/gamestatejson.hpp) based on the current screen, only the data it needs.

Finally, [pendings class](https://github.com/xaya/tfgsp/blob/main/src/pending.hpp) takes care of signaling the pending moves.

The rest are the files utilizing [Xaya SDK] (https://github.com/xaya/xaya_tutorials/wiki/libxayagame-Component-Relationships) which automatically takes care of all low-level stuff, like ability to process coins or rewind blocks on forks.


