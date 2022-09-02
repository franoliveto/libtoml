struct config {
    struct engine {
        char type[8]; /* SPI, USB */
        char device[16]; /* /dev/spidev0.0 */
        bool lorawan_public;
        int clksrc;
        int antenna_gain;
        bool full_duplex;
        struct fine_timestamp {
            bool enable;
            char mode[16]; /* all_sf, high_capacity */
        } timestamp;
        struct radio {
            bool enable;
            char type[8];
            int freq;
            double rssi_offset;
            struct tcomp {
                double coeff_a;
                double coeff_b;
                double coeff_c;
                double coeff_d;
                double coeff_e;
            } rssi_tcomp;
            struct tx {
                bool enable;
                int freq_min;
                int freq_max;
                struct gain_lut {
                    int rf_power;
                    int pa_gain;
                    int pwr_idx;
                } gains[16];
            } tx;
        } radios[NRADIOS];
        struct channel {
            bool enable;
            int radio;
            int if_freq;
            int bw; /* bandwidth */
            int sf; /* spreading factor */
            int dr; /* datarate */
            bool implicit_hdr;
            bool implicit_crc;
            int implicit_payload_len;
            int implicit_coderate;
        } channels[NIFCHANNELS], /* multiSF IF[0-7] channels */
            chan_lora, /* multiSF IF8 LoRa channel */
            chan_fsk; /* multiSF IF9 (G)FSK channel */
        int enable_sf[8]; /* list of enabled spreading factors */
    } engine;
    struct beacon {
        int period;
        struct freq {
            int hz;
            int nb;
            int step;
        } freq;
        int dr; /* datarate */
        int bw; /* bandwidth */
        int power;
        int id; /* information descriptor */
    } beacon;
    struct gps {
        char device[16];
        double lat;
        double lon;
        double alt;
    } gps;
} conf;


static const struct toml_keys_t[] = {
    {"device", string_t, .addr.string = conf.gps.device, .len = sizeof(conf.gps.device)},
    {NULL}
};

