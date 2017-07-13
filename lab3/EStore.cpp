#include <cassert>

#include "EStore.h"

using namespace std;


Item::
Item() : valid(false)
{
}

Item::
~Item()
{
}


EStore::
EStore(bool enableFineMode)
    : fineMode(enableFineMode)
{
    // TODO: Your code here.
    this->shipping_cost = 3;
    this->store_discount = 0;

    smutex_init(&this->lock);
    scond_init(&this->available);
}

EStore::
~EStore()
{
    // TODO: Your code here.
    smutex_destroy(&this->lock);
    scond_destroy(&this->available);
}

/*
 * ------------------------------------------------------------------
 * buyItem --
 *
 *      Attempt to buy the item from the store.
 *
 *      An item can be bought if:
 *          - The store carries it.
 *          - The item is in stock.
 *          - The cost of the item plus the cost of shipping is no
 *            more than the budget.
 *
 *      If the store *does not* carry this item, simply return and
 *      do nothing. Do not attempt to buy the item.
 *
 *      If the store *does* carry the item, but it is not in stock
 *      or its cost is over budget, block until both conditions are
 *      met (at which point the item should be bought) or the store
 *      removes the item from sale (at which point this method
 *      returns).
 *
 *      The overall cost of a purchase for a single item is defined
 *      as the current cost of the item times 1 - the store
 *      discount, plus the flat overall store shipping fee.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
buyItem(int item_id, double budget)
{
    assert(!fineModeEnabled());

    // TODO: Your code here.
    if (item_id < 0 || item_id > INVENTORY_SIZE) {
        return;
    }

    smutex_lock(&this->lock);

    Item item = this->inventory[item_id];

    if (!item.valid) {
        smutex_unlock(&this->lock);

        return;
    }

    while (item.quantity == 0 || item.price * (1 - item.discount) + this->shipping_cost > budget) {
        scond_wait(&this->available, &this->lock);
    }

    this->inventory[item_id].quantity--;

    smutex_unlock(&this->lock);
}

/*
 * ------------------------------------------------------------------
 * buyManyItem --
 *
 *      Attempt to buy all of the specified items at once. If the
 *      order cannot be bought, give up and return without buying
 *      anything. Otherwise buy the entire order at once.
 *
 *      The entire order can be bought if:
 *          - The store carries all items.
 *          - All items are in stock.
 *          - The cost of the the entire order (cost of items plus
 *            shipping for each item) is no more than the budget.
 *
 *      If multiple customers are attempting to buy at the same
 *      time and their orders are mutually exclusive (i.e., the
 *      two customers are not trying to buy any of the same items),
 *      then their orders must be processed at the same time.
 *
 *      For the purposes of this lab, it is OK for the store
 *      discount and shipping cost to change while an order is being
 *      processed.
 *
 *      The cost of a purchase of many items is the sum of the
 *      costs of purchasing each item individually. The purchase
 *      cost of an individual item is covered above in the
 *      description of buyItem.
 *
 *      Challenge: For bonus points, implement a version of this
 *      method that will wait until the order can be fulfilled
 *      instead of giving up. The implementation should be efficient
 *      in that it should not wake up threads unecessarily. For
 *      instance, if an item decreases in price, only threads that
 *      are waiting to buy an order that includes that item should be
 *      signaled (though all such threads should be signaled).
 *
 *      Challenge: For bonus points, ensure that the shipping cost
 *      and store discount does not change while processing an
 *      order.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
buyManyItems(vector<int>* item_ids, double budget)
{
    assert(fineModeEnabled());

    // TODO: Your code here.
}

/*
 * ------------------------------------------------------------------
 * addItem --
 *
 *      Add the item to the store with the specified quantity,
 *      price, and discount. If the store already carries an item
 *      with the specified id, do nothing.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
addItem(int item_id, int quantity, double price, double discount)
{
    // TODO: Your code here.
    if (item_id < 0 || item_id > INVENTORY_SIZE) {
        return;
    }

    smutex_lock(&this->lock);

    Item item = this->inventory[item_id];

    if (item.valid) {
        smutex_unlock(&this->lock);

        return;
    }

    item.valid = true;
    item.quantity = quantity;
    item.price = price;
    item.discount = discount;

    this->inventory[item_id] = item;

    scond_broadcast(&this->available, &this->lock);
    smutex_unlock(&this->lock);
}

/*
 * ------------------------------------------------------------------
 * removeItem --
 *
 *      Remove the item from the store. The store no longer carries
 *      this item. If the store is not carrying this item, do
 *      nothing.
 *
 *      Wake any waiters.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
removeItem(int item_id)
{
    // TODO: Your code here.
    if (item_id < 0 || item_id > INVENTORY_SIZE) {
        return;
    }

    smutex_lock(&this->lock);

    Item *item = &this->inventory[item_id];

    if (!item->valid) {
        smutex_unlock(&this->lock);

        return;
    }

    item->valid = false;

    smutex_unlock(&this->lock);
}

/*
 * ------------------------------------------------------------------
 * addStock --
 *
 *      Increase the stock of the specified item by count. If the
 *      store does not carry the item, do nothing. Wake any waiters.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
addStock(int item_id, int count)
{
    // TODO: Your code here.
    if (item_id < 0 || item_id > INVENTORY_SIZE) {
        return;
    }

    smutex_lock(&this->lock);

    Item *item = &this->inventory[item_id];

    if (!item->valid) {
        smutex_unlock(&this->lock);

        return;
    }

    item->quantity += count;

    scond_broadcast(&this->available, &this->lock);
    smutex_unlock(&this->lock);
}

/*
 * ------------------------------------------------------------------
 * priceItem --
 *
 *      Change the price on the item. If the store does not carry
 *      the item, do nothing.
 *
 *      If the item price decreased, wake any waiters.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
priceItem(int item_id, double price)
{
    // TODO: Your code here.
    if (item_id < 0 || item_id > INVENTORY_SIZE) {
        return;
    }

    smutex_lock(&this->lock);

    Item *item = &this->inventory[item_id];

    if (!item->valid) {
        smutex_unlock(&this->lock);

        return;
    }

    double original_price = item->price;

    item->price = price;

    if (original_price > price) {
        scond_broadcast(&this->available, &this->lock);
    }

    smutex_unlock(&this->lock);
}

/*
 * ------------------------------------------------------------------
 * discountItem --
 *
 *      Change the discount on the item. If the store does not carry
 *      the item, do nothing.
 *
 *      If the item discount increased, wake any waiters.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
discountItem(int item_id, double discount)
{
    // TODO: Your code here.
    if (item_id < 0 || item_id > INVENTORY_SIZE) {
        return;
    }

    smutex_lock(&this->lock);

    Item *item = &this->inventory[item_id];

    if (!item->valid) {
        smutex_unlock(&this->lock);

        return;
    }

    double original_discount = item->discount;

    item->discount = discount;

    if (original_discount > discount) {
        scond_broadcast(&this->available, &this->lock);
    }

    smutex_unlock(&this->lock);
}

/*
 * ------------------------------------------------------------------
 * setShippingCost --
 *
 *      Set the per-item shipping cost. If the shipping cost
 *      decreased, wake any waiters.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
setShippingCost(double cost)
{
    // TODO: Your code here.
    smutex_lock(&this->lock);

    double original_shipping_cost = this->shipping_cost;

    this->shipping_cost = cost;

    if (original_shipping_cost > cost) {
        scond_broadcast(&this->available, &this->lock);
    }

    smutex_unlock(&this->lock);
}

/*
 * ------------------------------------------------------------------
 * setStoreDiscount --
 *
 *      Set the store discount. If the discount increased, wake any
 *      waiters.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void EStore::
setStoreDiscount(double discount)
{
    // TODO: Your code here.
    smutex_lock(&this->lock);

    double original_store_discount = this->store_discount;

    this->store_discount = discount;

    if (original_store_discount < discount) {
        scond_broadcast(&this->available, &this->lock);
    }

    smutex_unlock(&this->lock);
}


