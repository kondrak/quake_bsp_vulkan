#ifndef Q3BSPSTATSUI_INCLUDED
#define Q3BSPSTATSUI_INCLUDED

#include "common/StatsUI.hpp"
#include "renderer/Font.hpp"

/*
 *  Quake III BSP stats display
 */

class Q3StatsUI : public StatsUI
{
public:
    Q3StatsUI(BspMap *map);

    ~Q3StatsUI()
    {
        delete m_font;
    }

    void OnRender(bool multithreaded);
    void RebuildPipeline();
private:
    Font *m_font = nullptr;
};

#endif