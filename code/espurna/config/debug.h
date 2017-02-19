#ifdef DEBUG_PORT
    #define DEBUG_MSG(...) { if (getBoard() != BOARD_ITEAD_SONOFF_DUAL) DEBUG_PORT.printf( __VA_ARGS__ ); }
    #define DEBUG_MSG_P(...) { if (getBoard() != BOARD_ITEAD_SONOFF_DUAL) { char buffer[81]; snprintf_P(buffer, 80, __VA_ARGS__ ); DEBUG_PORT.printf( buffer ); } }
#else
    #define DEBUG_MSG(...)
    #define DEBUG_MSG_P(...)
#endif
