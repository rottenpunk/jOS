/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OSCHAIN.H                                             */
/*                                                                           */
/*             Title:  Chain routine definitions and macros.                 */
/*                                                                           */
/*       Description:  Definitions and macros for the chain functions.       */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  04/25/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Chain Link structure...                                                   */
/*---------------------------------------------------------------------------*/

struct Link {
   struct Link  *Next;
   struct Link  *Prev;
   void         *Element;
};

typedef struct Link LINK;


/*---------------------------------------------------------------------------*/
/* Chain anchor structure...                                                 */
/*---------------------------------------------------------------------------*/

struct Anchor {
   struct Link  *First;
   struct Link  *Last;
};

typedef struct Anchor ANCHOR;



/*---------------------------------------------------------------------------*/
/* Useful macros for chain manipulation and traversing...                    */
/*---------------------------------------------------------------------------*/

#define ChainInit(l, e)     ((l)->Element = (void *) e)
#define ChainAnchorInit(a)  ((a)->First = (a)->Last = NULL)

#define ChainFirst(a)       ((a)->First == NULL ? NULL : (a)->First->Element)
#define ChainLast(a)        ((a)->Last  == NULL ? NULL : (a)->Last->Element)
#define ChainNext(l)        ((l)->Next  == NULL ? NULL : (l)->Next->Element)
#define ChainPrev(l)        ((l)->Prev  == NULL ? NULL : (l)->Prev->Element)

#define ChainQueue(a, x)    (Chain((a), (a)->Last, (x)))
#define ChainPush( a, x)    (Chain((a), 0l,        (x)))

#define ChainPop(a)      ((a)->First == NULL ? NULL : Unchain((a), (a)->First))



/*---------------------------------------------------------------------------*/
/* Useful macros for chain manipulation and traversing...                    */
/*---------------------------------------------------------------------------*/

void    Chain(      ANCHOR  *Anchor,
                    LINK    *Old,
                    LINK    *New);

void   *Unchain(    ANCHOR  *Anchor,
                    LINK    *Link );
