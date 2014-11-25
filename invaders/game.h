// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#pragma once

#include "config.h"

#include <memory>
#include <deque>
#include <functional>

namespace invaders
{
    class Level;

    class Game
    {
    public:
        struct invader {
            unsigned character;
            unsigned value;
            unsigned row;
            unsigned cell;
            unsigned identity;
        };
        struct missile {
            unsigned value;
            unsigned row; 
            unsigned cell;
            unsigned identity;
        };

        std::function<void (const invader&)> on_invader_hit;
        std::function<void (const invader&)> on_invader_spawn;        
        std::function<void (const invader&)> on_invader_victory;
        std::function<void (const missile&)> on_missile_fire;
        std::function<void (const missile&)> on_missile_prune;


        Game(unsigned width, unsigned height);
       ~Game();

        // advance game simulation by one increment
        void tick();

        // launch a missile at the current player position
        void fire(unsigned value);

        // start playing a level
        void play(Level& level);

        // move player up
        unsigned moveUp()
        {
            if (cursor_ >= 1)
                cursor_--;
            return cursor_;
        }

        // move player down
        unsigned moveDown()
        {
            if (cursor_ < height_-1)
                cursor_++;
            return cursor_;
        }

        // get game space width
        unsigned width() const 
        { return width_; }

        // get game space height
        unsigned height() const
        { return height_; }

        unsigned highScore() const
        { return highscore_; }

        unsigned score() const 
        { return score_; }

        const 
        std::deque<missile>& missiles() const 
        { return missiles_; }

        const
        std::deque<invader>& invaders() const 
        { return invaders_; }

        bool is_running() const 
        { return level_ != nullptr; }

    private:
        void pruneMissiles();
        void pruneInvaders();

    private:
        std::deque<missile> missiles_;
        std::deque<invader> invaders_;
        std::size_t tick_;
        unsigned width_;
        unsigned height_;
        unsigned cursor_;
        unsigned score_;
        unsigned identity_;
        unsigned highscore_;
    private:
        Level* level_;
    };

} // invaders