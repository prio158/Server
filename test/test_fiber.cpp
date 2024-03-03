//
// Created by czr on 24-2-27.
//
#include "Log.h"
#include "Fiber.h"

Server::Logger::ptr g_logger = LOG_ROOT();

void run_in_fiber() {
    LOGI(g_logger) << "run in fiber begin";
    Server::Fiber::YieldToHold();
    LOGI(g_logger) << "run in fiber end";
 }
/// each thread have a main fiber created by Fiber::GetThis()
/// [Server::Fiber::ptr fiber(new Server::Fiber(run_in_fiber))] can create a sub fiber
/// sub_fiber->swapIn(); //switch sub_fiber execute, and suspend main fiber
int main() {
    {
        /// init main fiber
        Server::Fiber::GetThis();
        LOGI(g_logger) << "main fiber begin";
        ///create a sub fiber with func
        Server::Fiber::ptr fiber(new Server::Fiber(run_in_fiber));
        fiber->swapIn();
        LOGI(g_logger) << "run in fiber swapOut after";
        fiber->swapIn();
        LOGI(g_logger) << "main fiber end";
    }
    LOGI(g_logger) << "main fiber end2";
}