#include "Grid.hpp"
#include <GeoQik/GeoQik.hpp>
#include <array>
#include <barrier>
#include <cassert>
#include <cmath>
#include <thread>

int
main()
{
    // Example to provoke contention in the GeoQik API.
    //
    // This example creates a spiral of lines and points, demonstrating the use of the GeoQik API
    // with multiple threads adding lines and points concurrently.

    geoqik_settings_t settings;
    geoqik_create_default_settings(&settings);
    const std::uint32_t initialGeometryCapacity = 10000;
    settings.initialPointCapacity = initialGeometryCapacity;
    settings.initialLineCapacity = initialGeometryCapacity;

    geoqik_init_with_settings(&settings, nullptr);

    constexpr float pointSize = 5.0F;
    constexpr float lineWidth = 2.0F;
    geoqik_set_point_size(pointSize);
    geoqik_set_line_width(lineWidth);

    geoqik_draw();

    constexpr float colorMin = 0.0F;
    constexpr float colorMax = 1.0F;
    geoqik_add_point_with_color(0.0, 0.0, 0.0, colorMax, colorMin, colorMin, colorMax);
    geoqik_add_point_with_color(1.0, 0.0, 0.0, colorMin, colorMax, colorMin, colorMax);
    geoqik_add_point_with_color(0.0, 1.0, 0.0, colorMin, colorMin, colorMax, colorMax);
    geoqik_add_point_with_color(0.0, 0.0, 1.0, colorMin, colorMin, colorMin, colorMax);

    geoqik_add_line_with_color(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, colorMax, colorMin, colorMin, colorMax);
    geoqik_add_line_with_color(0.0, 0.0, 0.0, 0.0, 1.0, 0.0, colorMin, colorMax, colorMin, colorMax);
    geoqik_add_line_with_color(0.0, 0.0, 0.0, 0.0, 0.0, 1.0, colorMin, colorMin, colorMax, colorMax);

    constexpr double gridSize = 500.0;
    constexpr double gridSpacing = 10.0;
    auto grid = geoqik::examples::create_grid(gridSize, gridSpacing);
    geoqik::examples::add_grid(grid);

    // Use multiple threads to provoke contention
    constexpr std::size_t threadCount = 5;
    std::array<std::thread, threadCount> threads;
    const std::size_t numThreads = threads.size();

    // Synchronization point to ensure all threads start adding lines at the same time
    std::barrier syncPoint(static_cast<std::int64_t>(numThreads));

    // Produces lines and adds them to geoqik.
    auto addSpiralLines = [&syncPoint](const std::size_t startIdx, const std::size_t endIdx) {
        syncPoint.arrive_and_wait(); // Wait for all threads to reach this point

        // Parameters for the Archimedean spiral
        const double a = 0.1;          // Starting radius
        const double b = 0.03;         // Growth factor
        const double c = 0.005;        // Z-axis ascent rate
        const double lineLength = 0.3; // Length of the outward lines
        const double thetaStep = 0.1;  // Angular step for discretization

        double prevX = 0;
        double prevY = 0;
        double prevZ = 0;
        bool firstPoint = true;

        for (std::size_t i = startIdx; i < endIdx; ++i) {
            // Calculate parameter along the spiral
            const double theta = static_cast<double>(i) * thetaStep;

            // Calculate point on the spiral
            const double r = a + (b * theta);
            const double spiralX = r * std::cos(theta);
            const double spiralY = r * std::sin(theta);
            const double spiralZ = c * theta;

            // Calculate tangent vector (derivative of the spiral)
            const double dxdtheta = (b * std::cos(theta)) - (r * std::sin(theta));
            const double dydtheta = (b * std::sin(theta)) + (r * std::cos(theta));
            const double dzdtheta = c;

            // Normalize tangent vector
            const double tangentMagnitude =
                std::sqrt((dxdtheta * dxdtheta) + (dydtheta * dydtheta) + (dzdtheta * dzdtheta));
            const double tx = dxdtheta / tangentMagnitude;
            const double ty = dydtheta / tangentMagnitude;
            const double tz = dzdtheta / tangentMagnitude;

            double nx = -spiralX;
            double ny = -spiralY;
            double nz = 0.0;

            // Make normal vector perpendicular to tangent
            const double dot = (nx * tx) + (ny * ty) + (nz * tz);
            nx -= dot * tx;
            ny -= dot * ty;
            nz -= dot * tz;

            const double normalMagnitude = std::sqrt((nx * nx) + (ny * ny) + (nz * nz));
            nx /= normalMagnitude;
            ny /= normalMagnitude;
            nz /= normalMagnitude;

            // Calculate binormal vector to complete the frame
            // const double bx = ty*nz - tz*ny;
            // const double by = tz*nx - tx*nz;
            // const double bz = tx*ny - ty*nx;

            // Color based on position along the spiral
            constexpr float colorOffset = 0.5F;
            constexpr float colorScale = 0.5F;
            constexpr float piApprox = 3.14F;
            geoqik_set_line_color(colorOffset + (std::sin(static_cast<float>(theta)) * colorScale),
                                  colorOffset + (std::cos(static_cast<float>(theta)) * colorScale),
                                  colorOffset + (std::sin(static_cast<float>(theta) + piApprox) * colorScale),
                                  colorMax);

            // Draw the discretized spiral by connecting consecutive points
            if (!firstPoint) {
                geoqik_add_line(prevX, prevY, prevZ, spiralX, spiralY, spiralZ);
            }
            firstPoint = false;

            // Draw line from spiral point outward along normal vector (torsion-free sweep)
            geoqik_add_line(spiralX,
                            spiralY,
                            spiralZ,
                            spiralX - (nx * lineLength),
                            spiralY - (ny * lineLength),
                            spiralZ - (nz * lineLength));

            prevX = spiralX;
            prevY = spiralY;
            prevZ = spiralZ;
        }
    };

    const std::size_t numberOfSteps = 300000;
    const std::size_t stride = numberOfSteps / numThreads;
    for (std::size_t i = 0; i < threads.size(); ++i) {
        const std::size_t startIdx = i * stride;
        const std::size_t endIdx = startIdx + stride;
        threads[i] = std::thread(addSpiralLines, startIdx, endIdx);
    }

    for (auto& thread: threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    geoqik_wait_for_exit_and_cleanup();

    return 0;
}
