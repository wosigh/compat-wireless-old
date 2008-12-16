
struct ar9170_mib_stats {
	u32 ackrcv_bad;
	u32 rts_bad;
	u32 rts_good;
	u32 fcs_bad;
	u32 beacons;
};

struct ar9170_node_stats {
	u32 ns_avgbrssi; /* average beacon rssi */
	u32 ns_avgrssi; /* average data rssi */
	u32 ns_avgtxrssi; /* average tx rssi */
};

/* Adaptive Noise Immunity stats */
struct ar9170_ani_stats {
	u32 ast_ani_niup;   /* ANI increased noise immunity */
	u32 ast_ani_nidown; /* ANI decreased noise immunity */
	u32 ast_ani_spurup; /* ANI increased spur immunity */
	u32 ast_ani_spurdown;/* ANI descreased spur immunity */
	u32 ast_ani_ofdmon; /* ANI OFDM weak signal detect on */
	u32 ast_ani_ofdmoff;/* ANI OFDM weak signal detect off */
	u32 ast_ani_cckhigh;/* ANI CCK weak signal threshold high */
	u32 ast_ani_ccklow; /* ANI CCK weak signal threshold low */
	u32 ast_ani_stepup; /* ANI increased first step level */
	u32 ast_ani_stepdown;/* ANI decreased first step level */
	u32 ast_ani_ofdmerrs;/* ANI cumulative ofdm phy err count */
	u32 ast_ani_cckerrs;/* ANI cumulative cck phy err count */
	u32 ast_ani_reset;  /* ANI parameters zero'd for non-STA */
	u32 ast_ani_lzero;  /* ANI listen time forced to zero */
	u32 ast_ani_lneg;   /* ANI listen time calculated < 0 */
	struct ar9170_mib_stats ast_mibstats;   /* MIB counter stats */
	struct ar9170_node_stats ast_nodestats;  /* Latest rssi stats from driver */
};
