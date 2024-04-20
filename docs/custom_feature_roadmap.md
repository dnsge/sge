# Custom Feature Roadmap

## Planning / Infra 

- [x] Switch to CMake build system
- [x] Split application into server and client binaries
- [x] Design multiplayer component architecture
- [x] Design wire protocol
- [x] Bring in Boost & msgpack-c
- [x] Build out TCP socket messaging capabilities
- [x] Design Lua API
- [x] Build out replicated `Transform` C++ component

## Multiplayer Capabilities
- [x] Implement `MessageHello` / `MessageWelcome` flow
- [x] Implement `MessageLoadScene`
- [x] Implement `MessageTickReplicate`
- [x] Implement runtime actor instantiation / `MessageTickReplicateAck`
- [x] Solve problem of runtime components
- [x] Functional changing of scenes at runtime
- [x] Player join/leave API (through Event API?)
- [x] Synchronized Event API
- [x] Client ownership of actors
- [x] Configuration of host/port to connect/host on
- [x] Synchronize actor destroy

## Beyond
- [x] Client support for single-player scenes (for example, menu)
- [x] Build simple 1v1 shooter game
- [ ] Build simple chat game
- [ ] Un-break physics?
- [x] Integrated all-in-one build for single-player games
