set(TARGET_NAME "sequencer_synth")

if (NOT TARGET ${TARGET_NAME})
    add_library(${TARGET_NAME} INTERFACE)

    target_sources(${TARGET_NAME} INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/synth/synth.c
            ${CMAKE_CURRENT_LIST_DIR}/sound_pwm/sound_pwm.c
            ${CMAKE_CURRENT_LIST_DIR}/sound_i2s/sound_i2s.c
            ${CMAKE_CURRENT_LIST_DIR}/sequencer/sequencer.c
    )
    
    pico_generate_pio_header(${TARGET_NAME} ${CMAKE_CURRENT_LIST_DIR}/sound_i2s/sound_i2s_16bits.pio)
    pico_generate_pio_header(${TARGET_NAME} ${CMAKE_CURRENT_LIST_DIR}/sound_i2s/sound_i2s_8bits.pio)

    target_include_directories(${TARGET_NAME} INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}
            ${CMAKE_CURRENT_LIST_DIR}/synth
            ${CMAKE_CURRENT_LIST_DIR}/sequencer
            ${CMAKE_CURRENT_LIST_DIR}/sound_i2s
            ${CMAKE_CURRENT_LIST_DIR}/sound_pwm
    )

    target_link_libraries(${TARGET_NAME} INTERFACE
        pico_stdlib
        hardware_pwm
        hardware_irq
        hardware_dma
        hardware_pio
    )
endif()
