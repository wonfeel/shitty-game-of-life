# cuda-life-early

An early, unoptimized prototype of Conway's Game of Life on CUDA + OpenGL +
GLFW — no chunking, no thread pool, no clean split between simulation and
rendering, debug info printed straight to the console because that was
faster than building a UI for it.

This is the rough first attempt [Tessera](https://github.com/wonfeel/Tessera)
later grew out of. Everything this prototype got wrong on purpose or by
accident — no chunking (the whole grid simulates every step, live or not),
sim and render sharing one thread, no separation between "engine" and
"demo" — is exactly what Tessera's chunk/thread-pool/backend-interface
design exists to fix. Kept here as the before-picture.

## Controls

```
Space   step (deliberately latent, so you have time to get bored)
N       step
C       new field (seeded from current time in milliseconds - rand() is for the weak)
X       clear field
Esc     quit
```

## Running it

Built for a specific machine at the time — no portable build instructions
were ever written for this one. See [Tessera](https://github.com/wonfeel/Tessera)
for the version that actually builds anywhere.
