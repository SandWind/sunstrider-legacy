
#ifndef TRINITY_NGRID_H
#define TRINITY_NGRID_H

/** NGrid is nothing more than a wrapper of the Grid with an NxN cells
 */

#include "Grid.h"
#include "GridReference.h"
#include "Timer.h"

#define DEFAULT_VISIBILITY_NOTIFY_PERIOD      1000

class GridInfo
{
public:
    GridInfo();
    GridInfo(time_t expiry, bool unload = true);

	const TimeTracker& getTimeTracker() const { return i_timer; }
    bool getUnloadLock() const { return i_unloadActiveLockCount || i_unloadExplicitLock || i_unloadReferenceLock; }
	void setUnloadExplicitLock(bool on) { i_unloadExplicitLock = on; }
    void setUnloadReferenceLock(bool on) { i_unloadReferenceLock = on; }
	void incUnloadActiveLock() { ++i_unloadActiveLockCount; }
	void decUnloadActiveLock() { if (i_unloadActiveLockCount) --i_unloadActiveLockCount; }

	void setTimer(const TimeTracker& pTimer) { i_timer = pTimer; }
	void ResetTimeTracker(time_t interval) { i_timer.Reset(interval); }
	void UpdateTimeTracker(time_t diff) { i_timer.Update(diff); }
	PeriodicTimer& getRelocationTimer() { return vis_Update; }
private:
	TimeTracker i_timer;
	PeriodicTimer vis_Update;

	uint16 i_unloadActiveLockCount : 16;                 // lock from active object spawn points (prevent clone loading)
	bool   i_unloadExplicitLock    : 1;                  // explicit manual lock or config setting
    bool   i_unloadReferenceLock   : 1;                  // lock from instance map copy
};

typedef enum
{
	GRID_STATE_INVALID = 0,
	GRID_STATE_ACTIVE  = 1,
	GRID_STATE_IDLE    = 2,
	//GRID_STATE_REMOVAL = 3, removed, never unload
	//MAX_GRID_STATE     = 4
	MAX_GRID_STATE     = 3,
} grid_state_t;

template
<
unsigned int N,
class ACTIVE_OBJECT,
class WORLD_OBJECT_TYPES,
class GRID_OBJECT_TYPES,
class ThreadModel = Trinity::SingleThreaded<ACTIVE_OBJECT>
>
class NGrid
{
    public:

        typedef Grid<ACTIVE_OBJECT, WORLD_OBJECT_TYPES, GRID_OBJECT_TYPES, ThreadModel> GridType;
        NGrid(uint32 id, int32 x, int32 y, time_t expiry, bool unload = true) :
            i_gridId(id), i_GridInfo(GridInfo(expiry, unload)), i_x(x), i_y(y), i_cellstate(GRID_STATE_INVALID), i_GridObjectDataLoaded(false)
            {
            }

		GridType& GetGridType(const uint32 x, const uint32 y)
		{
			ASSERT(x < N && y < N);
			return i_cells[x][y];
		}

		GridType const& GetGridType(const uint32 x, const uint32 y) const
		{
			ASSERT(x < N && y < N);
			return i_cells[x][y];
		}

        inline const uint32& GetGridId(void) const { return i_gridId; }
        inline void SetGridId(const uint32 id)  { i_gridId = id; }
		grid_state_t GetGridState(void) const { return i_cellstate; }
		void SetGridState(grid_state_t s) { i_cellstate = s; }
        inline int32 getX() const { return i_x; }
        inline int32 getY() const { return i_y; }

        void link(GridRefManager<NGrid<N, ACTIVE_OBJECT, WORLD_OBJECT_TYPES, GRID_OBJECT_TYPES, ThreadModel> >* pTo)
        {
            i_Reference.link(pTo, this);
        }
        bool isGridObjectDataLoaded() const { return i_GridObjectDataLoaded; }
        void setGridObjectDataLoaded(bool pLoaded) { i_GridObjectDataLoaded = pLoaded; }

		GridInfo* getGridInfoRef() { return &i_GridInfo; }
		const TimeTracker& getTimeTracker() const { return i_GridInfo.getTimeTracker(); }
		bool getUnloadLock() const { return i_GridInfo.getUnloadLock(); }
		void setUnloadExplicitLock(bool on) { i_GridInfo.setUnloadExplicitLock(on); }
		void setUnloadReferenceLock(bool on) { i_GridInfo.setUnloadReferenceLock(on); }
		void incUnloadActiveLock() { i_GridInfo.incUnloadActiveLock(); }
		void decUnloadActiveLock() { i_GridInfo.decUnloadActiveLock(); }
		void ResetTimeTracker(time_t interval) { i_GridInfo.ResetTimeTracker(interval); }
		void UpdateTimeTracker(time_t diff) { i_GridInfo.UpdateTimeTracker(diff); }

		/*
        template<class SPECIFIC_OBJECT> void AddWorldObject(const uint32 x, const uint32 y, SPECIFIC_OBJECT *obj)
        {
            getGridType(x, y).AddWorldObject(obj);
        }

        template<class SPECIFIC_OBJECT> void RemoveWorldObject(const uint32 x, const uint32 y, SPECIFIC_OBJECT *obj)
        {
            getGridType(x, y).RemoveWorldObject(obj);
        }
		*/


        //This gets the player count in grid
        //I disable this to avoid confusion (active object usually means something else)
        /*
        uint32 GetActiveObjectCountInGrid() const
        {
            uint32 count = 0;
            for (uint32 x = 0; x < N; ++x)
                for (uint32 y = 0; y < N; ++y)
                    count += i_cells[x][y].ActiveObjectsInGrid();
            return count;
        }
        */

		/*
        template<class SPECIFIC_OBJECT> bool AddGridObject(const uint32 x, const uint32 y, SPECIFIC_OBJECT *obj)
        {
            return getGridType(x, y).AddGridObject(obj);
        }

        template<class SPECIFIC_OBJECT> bool RemoveGridObject(const uint32 x, const uint32 y, SPECIFIC_OBJECT *obj)
        {
            return getGridType(x, y).RemoveGridObject(obj);
        }
		*/

		// Visit all Grids (cells) in NGrid (grid)
		template<class T, class TT>
		void VisitAllGrids(TypeContainerVisitor<T, TypeMapContainer<TT> > &visitor)
		{
			for (uint32 x = 0; x < N; ++x)
				for (uint32 y = 0; y < N; ++y)
					GetGridType(x, y).Visit(visitor);
		}

		// Visit a single Grid (cell) in NGrid (grid)
		template<class T, class TT>
		void VisitGrid(const uint32 x, const uint32 y, TypeContainerVisitor<T, TypeMapContainer<TT> > &visitor)
		{
			GetGridType(x, y).Visit(visitor);
		}

		/*
		template<class T, class TT> void Visit(TypeContainerVisitor<T, TypeMapContainer<TT> > &visitor)
		{
			for (unsigned int x = 0; x < N; ++x)
				for (unsigned int y = 0; y < N; ++y)
					GetGridType(x, y).Visit(visitor);
		}
		*/

        template<class T>
        uint32 GetWorldObjectCountInNGrid() const
        {
            uint32 count = 0;
            for (uint32 x = 0; x < N; ++x)
                for (uint32 y = 0; y < N; ++y)
                    count += i_cells[x][y].template GetWorldObjectCountInGrid<T>();
            return count;
        }

    private:

        uint32 i_gridId;
		GridInfo i_GridInfo;
        GridReference<NGrid<N, ACTIVE_OBJECT, WORLD_OBJECT_TYPES, GRID_OBJECT_TYPES, ThreadModel> > i_Reference;
        int32 i_x;
        int32 i_y;
		grid_state_t i_cellstate;
        GridType i_cells[N][N];
        bool i_GridObjectDataLoaded;
};
#endif

