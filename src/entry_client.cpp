int client_main(int argc, char** argv);

#if defined(PLATFORM_ESP32)
extern "C" void app_main(void) {
    client_main(0, nullptr);
}
#else
int main(int argc, char** argv) {
    return client_main(argc, argv);
}
#endif
