#ifdef DEBUG_PORT
    #define DEBUG_MSG(...) if (getBoard() != BOARD_ITEAD_SONOFF_DUAL) DEBUG_PORT.printf( __VA_ARGS__ )
#else
    #define DEBUG_MSG(...)
#endif
