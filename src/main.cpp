#include <iostream>
#include <cmath>

#include <SFML/Graphics.hpp>

// This function converts decimal degrees to radians
float deg2rad(float deg)
{
    return (deg * M_PI / 180);
}

//  This function converts radians to decimal degrees
float rad2deg(float rad)
{
    return (rad * 180 / M_PI);
}

// Latitude-Longitude coordinates
struct LatLon
{
    // 90° at north pole, -90° at south pole
    float lat;
    // 0° through London, ±180 at the other side, positive goes west
    float lon;

    // Compute the length of the path between two points along a great circle.
    //
    // https://en.wikipedia.org/wiki/Haversine_formula
    float spherical_distance(const LatLon &other) const
    {
        const auto earth_radius_km = 6371.f;

        const auto lat1r = deg2rad(lat);
        const auto lon1r = deg2rad(lon);
        const auto lat2r = deg2rad(other.lat);
        const auto lon2r = deg2rad(other.lon);
        const auto u = sinf((lat2r - lat1r) / 2);
        const auto v = sinf((lon2r - lon1r) / 2);

        return 2.0 * earth_radius_km * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
    }

    // Map to x-y representation on the azimuthal equidistant projection
    sf::Vector2f to_azimuthal_equidistant() const
    {
        const auto r = -(lat - 90.f) / 180.f;
        const auto th = deg2rad(lon);
        return r * sf::Vector2f(-sin(th), cos(th));
    }

    // Compute LatLon from x-y azimuthal equidistant projection coordinates
    static LatLon from_azimuthal_equidistant(sf::Vector2f coords)
    {
        const auto r = sqrtf(coords.x * coords.x + coords.y * coords.y);
        const auto th = atan2f(-coords.x, coords.y);

        const auto lat = -r * 180.f + 90.f;
        const auto lon = rad2deg(th);

        return LatLon{lat, lon};
    }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 800), "Flat Earth");

    // Load world map texture
    sf::Texture texture;
    if (!texture.loadFromFile("../../map.jpg"))
    {
        std::cerr << "Can't load map.jpg" << std::endl;
        return 1;
    }
    texture.setSmooth(true);

    // World map sprite
    sf::Sprite world_map;
    world_map.setTexture(texture);
    world_map.setScale(sf::Vector2f(800. / 2058, 800. / 2058));

    // Washington (because why not)
    auto point = LatLon{47.7511, 120.7401};

    // Sun position marker
    //
    // Sun is directly overhead at this point.
    sf::CircleShape marker(10);
    marker.setOrigin(sf::Vector2f(10, 10));
    marker.setFillColor(sf::Color(220, 220, 30));

    // 1px-wide rectangle for illumination computation
    const auto step = 1.f;
    sf::RectangleShape tile(sf::Vector2f(step, step));
    tile.setOrigin(sf::Vector2f(step / 2.f, step / 2.f));
    tile.setFillColor(sf::Color(0, 0, 0, 220));

    while (window.isOpen())
    {
        // Stop app if window is closed
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }

        // Clear and overlay world map
        window.clear(sf::Color::Black);
        window.draw(world_map);

        // Put sun at mouse position if space is pressed
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
        {
            const auto pixel = sf::Mouse::getPosition(window);
            const auto coord = window.mapPixelToCoords(pixel);

            point = LatLon::from_azimuthal_equidistant((coord - sf::Vector2f(400, 400)) / 400.f);
        }

        // Render illumination
        for (float x = 0.f; x < 800.f; x += step)
        {
            for (float y = 0.f; y < 800.f; y += step)
            {
                // LatLon coordinate of the given 1px tile
                const auto tile_coords = LatLon::from_azimuthal_equidistant((sf::Vector2f(x, y) - sf::Vector2f(400, 400)) / 400.f);

                // Don't put the shadow if the point is outside of the map or the distance from marker is more than 1/4 of earth circumference
                if (tile_coords.lat < -90. || point.spherical_distance(tile_coords) < 40075. / 4.)
                {
                    continue;
                }

                // reposition & draw tile
                tile.setPosition(sf::Vector2f(x, y));
                window.draw(tile);
            }
        }

        // Put the marker showing where the sun is directly overhead
        marker.setPosition(sf::Vector2f(400, 400) + 400.f * point.to_azimuthal_equidistant());
        window.draw(marker);

        window.display();
    }

    return 0;
}