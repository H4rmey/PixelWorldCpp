#pragma once
#include "protocol.hpp"
#include <SFML/Graphics.hpp>
#include <enet/enet.h>
#include <string>

class Client
{
  private:
    ENetHost *m_host = nullptr;
    ENetPeer *m_peer = nullptr;

    void on_packet_receive(ENetEvent &event, sf::VertexArray &worldPixels);

  public:
    Client() = default;
    ~Client();

    bool init();
    void stop();
    bool connect(const std::string &host_ip, uint16_t port);

    // Checks for new packets and updates the SFML VertexArray
    void poll_events(sf::VertexArray &worldPixels);

    // Sends the brush input to the server
    void send_brush_click(int gx, int gy, int brush_size, uint8_t element_type, bool erase);

    void send_client_position(int gx, int gy);
};
