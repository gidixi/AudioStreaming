# Opus Audio Streaming (C++) — Client/Server UDP

Client C++ che cattura audio **mono 16‑bit 48 kHz**, lo codifica con **Opus** a frame **20 ms** e lo invia via **UDP**; Server che riceve, decodifica e scrive su **file .pcm** (oppure verso un **ring buffer** per un adapter di trascrizione). Codice modulare (interfacce a classi), portabile su Linux/Windows/macOS; stub per **ESP32 (I2S)**.

---

## Struttura del progetto

```
include/
  audio_source.h     # interfaccia sorgente audio (PCM S16 mono)
  transport.h        # interfacce trasporto UDP (sender/receiver)
  opus_codec.h       # wrapper Opus encoder/decoder
  sink.h             # interfaccia Sink (file, ring buffer)
  proto.h            # header pacchetto + costanti audio (48k / 20ms)
src/
  client_main.cpp    # loop di cattura+Opus+invio (senza main)
  server_main.cpp    # loop di ricezione+Opus->PCM+sink (senza main)
  entry_client.cpp   # main() che chiama client_main(...)
  entry_server.cpp   # main() che chiama server_main(...)
  transport_udp.cpp  # implementazione UDP (POSIX/WinSock)
  audio_portaudio.cpp# sorgente PortAudio (desktop)
  audio_esp32.cpp    # sorgente I2S (stub ESP32)
  opus_codec.cpp     # implementazione wrapper Opus
  sink_file.cpp      # sink file .pcm
CMakeLists.txt
```

> Nota: `entry_client.cpp` e `entry_server.cpp` forniscono la `main()`. Se non ci fossero, creali come indicato più sotto.

---

## Dipendenze

### Linux (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libopus-dev portaudio19-dev
# utili per test/ascolto:
sudo apt install alsa-utils ffmpeg pavucontrol
```

### Fedora / RHEL

```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake pkgconf-pkg-config opus-devel portaudio-devel alsa-utils ffmpeg pavucontrol
```

### Arch / Manjaro

```bash
sudo pacman -S base-devel cmake opus portaudio alsa-utils ffmpeg pavucontrol
```

### macOS (Homebrew)

```bash
brew install cmake opus portaudio ffmpeg
```

### Windows

- Compila con **MSYS2** *oppure* Visual Studio + **vcpkg**.
- Librerie: `opus`, `portaudio` (+ link a `ws2_32` per WinSock).  
  Esempio CMake (solo quando su Windows):
  ```cmake
  if (WIN32)
    target_link_libraries(opus_client PRIVATE ws2_32)
    target_link_libraries(opus_server PRIVATE ws2_32)
  endif()
  ```

---

## Build (desktop)

```bash
mkdir -p build && cd build
cmake -DPLATFORM_DESKTOP=ON ..
cmake --build . --config Release -j
```

Se non hai ancora gli entry point, aggiungili:

**`src/entry_client.cpp`**
```cpp
int client_main(int argc, char** argv);
int main(int argc, char** argv) { return client_main(argc, argv); }
```

**`src/entry_server.cpp`**
```cpp
int server_main(int argc, char** argv);
int main(int argc, char** argv) { return server_main(argc, argv); }
```

> Il CMake già linka `Threads::Threads`. Su Windows aggiungi `ws2_32` come sopra.

---

## Esecuzione & Test rapidi

### 1) Test locale (loopback)

**Terminale A — server:**
```bash
mkdir -p out
./opus_server 0.0.0.0 50000 out
```
**Terminale B — client:**
```bash
# opzionale: lista dispositivi input disponibili
LIST_DEVICES=1 ./opus_client 42 127.0.0.1 50000

# scegli un indice con inputs>0 e forzalo, es. 2
PA_DEVICE_INDEX=2 ./opus_client 42 127.0.0.1 50000
```

Dovresti vedere su server:
```
[new client] id=42 from=127.0.0.1:xxxxx file=out/client_42.pcm
```

**Ascolta il file mentre cresce:**
```bash
tail -f -c +0 out/client_42.pcm | aplay -f S16_LE -r 48000 -c 1
```
oppure
```bash
ffmpeg -f s16le -ar 48000 -ac 1 -i out/client_42.pcm out/client_42.wav
```

### 2) Test su LAN (due macchine)

Sul **server**:
```bash
ip addr show | grep 'inet '
sudo ufw allow 50000/udp   # se usi UFW
./opus_server 0.0.0.0 50000 out
```
Sul **client** (altra macchina):
```bash
PA_DEVICE_INDEX=<idx> ./opus_client 100 <IP_DEL_SERVER> 50000
```

### 3) Multi‑client

Apri più client con ID diversi:
```bash
PA_DEVICE_INDEX=2 ./opus_client 1 127.0.0.1 50000 &
PA_DEVICE_INDEX=2 ./opus_client 2 127.0.0.1 50000 &
```
Il server creerà `out/client_1.pcm`, `out/client_2.pcm`, …

---

## Parametri & Ambiente utili

- **Bitrate Opus**: impostato a ~32 kbps in `client_main.cpp` (`makeOpusEncoder(..., 32000)`).
- **Frame size**: 20 ms (960 campioni @ 48 kHz).
- **Env:**  
  - `LIST_DEVICES=1` → stampa la lista dei device PortAudio all’avvio del client.  
  - `PA_DEVICE_INDEX=<n>` → forza l’input device.  
- **Formato file server**: `out/client_<id>.pcm` (S16LE, 48 kHz, mono).

---

## Integrazione con trascrizione (adapter)

Lato server, al posto del file puoi usare un **RingBufferSink** (vedi `include/sink.h`) e creare un piccolo thread che legge chunk (es. 1 s = 48000 campioni) e li passa al tuo modello (Whisper, Vosk, cloud, ecc.). Esempio schematico:

```cpp
auto rbs = std::make_shared<RingBufferSink>(48000 * 10); // 10s
ctx.sink.reset(rbs.get());

// thread consumer (pseudo)
std::vector<int16_t> chunk(48000);
while (running) {
  size_t n = rbs->read(chunk.data(), chunk.size());
  if (n == chunk.size()) { asr.process(chunk.data(), n); }
}
```

---

## ESP32 (note rapide)

- Usa **ESP‑IDF**; implementa `audio_esp32.cpp` con `i2s_driver_install`, `i2s_read` per ottenere PCM S16 mono 48 kHz.
- Porta/compila **libopus** per ESP32 (esistono ricette/porting IDF). Mantieni lo stesso header di rete (lwIP UDP).
- Definisci `PLATFORM_ESP32` e fornisci lo `IAudioSource` basato su I2S. Il resto dell’architettura non cambia.

---

## Troubleshooting

- **Warning ALSA/JACK** in console: spesso innocui. Preferisci PulseAudio o forza un device valido con `PA_DEVICE_INDEX`. Usa `pavucontrol` → “Recording/Input” per verificare livello e sorgente.
- **Il file non cresce**: controlla che il client parli col server giusto (IP/porta), firewall aperto (UDP 50000), e che ci sia input (usa `LIST_DEVICES=1` e scegli un device con `inputs>0`).
- **`undefined reference to main`**: mancano gli entry (`entry_client.cpp` / `entry_server.cpp`) — vedi sezione Build.
- **`htobe64`/`htonl` mancanti o “namespaced”**: assicurati che gli include di sistema **non** siano dentro `namespace proto` (vedi `include/proto.h`).
- **Dipendenze non trovate**: installa i pacchetti dev (opus/portaudio) e `pkg-config` come indicato sopra.

---

## Licenze

- **Opus** è (c) Xiph.Org; **PortAudio** è BSD‑style. Questo progetto d’esempio è senza licenza specificata: aggiungi la tua licenza se lo redistribuisci.

Buon hacking! ✨
