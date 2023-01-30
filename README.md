only compile jampgamei386.so

```
cmake . -DBuildMPEngine=OFF -DBuildMPDed=OFF -DBuildMPCGame=OFF -DBuildMPUI=OFF -DBuildSPEngine=OFF -DBuildSPGame=OFF -DForcei386Compile=ON
cmake --build .
```
