using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

public class Inventory
{
	public Action m_onChanged;

	private string m_name = "";

	private Sprite m_bkg;

	private List<ItemDrop.ItemData> m_inventory = new List<ItemDrop.ItemData>();

	private int m_width = 4;

	private int m_height = 4;

	private float m_totalWeight;

	private bool m_cheatedPopup;

	private readonly bool m_temoraryInventory;

	public Inventory(string name, Sprite bkg, int w, int h)
	{
		m_temoraryInventory = false;
		m_bkg = bkg;
		m_name = name;
		m_width = w;
		m_height = h;
	}

	public Inventory(bool _)
	{
		m_temoraryInventory = true;
	}

	private bool AddItem(ItemDrop.ItemData item, int amount, int x, int y, bool skipValidPositionCheck = false)
	{
		amount = Mathf.Min(amount, item.m_stack);
		if ((x < 0 || y < 0 || x >= m_width || (y >= m_height && !skipValidPositionCheck)) && !m_temoraryInventory)
		{
			return false;
		}
		bool flag = false;
		bool cheatedStateChanged = item.m_cheated && !Achievements.IsCheatedAtAll() && Player.m_localPlayer?.GetInventory() == this;
		ItemDrop.ItemData itemAt = GetItemAt(x, y);
		if (itemAt != null)
		{
			if (!itemAt.IsSameType(item))
			{
				return false;
			}
			int num = itemAt.m_shared.m_maxStackSize - itemAt.m_stack;
			if (num <= 0)
			{
				return false;
			}
			int num2 = Mathf.Min(num, amount);
			itemAt.m_stack += num2;
			item.m_stack -= num2;
			flag = num2 == amount;
			ZLog.Log("Added to stack" + itemAt.m_stack + " " + item.m_stack);
		}
		else
		{
			ItemDrop.ItemData itemData = item.Clone();
			itemData.m_stack = amount;
			itemData.m_gridPos = new Vector2i(x, y);
			m_inventory.Add(itemData);
			item.m_stack -= amount;
			flag = true;
		}
		if (Player.m_localPlayerExists)
		{
			Changed(success: true, cheatedStateChanged);
		}
		return flag;
	}

	public bool CanAddItem(GameObject prefab, int stack = -1)
	{
		ItemDrop component = prefab.GetComponent<ItemDrop>();
		if (component == null)
		{
			return false;
		}
		return CanAddItem(component.m_itemData, stack);
	}

	public bool CanAddItem(ItemDrop.ItemData item, int stack = -1)
	{
		if (stack <= 0)
		{
			stack = item.m_stack;
		}
		return FindFreeStackSpace(item.m_shared.m_name, item.m_worldLevel) + (m_width * m_height - m_inventory.Count) * item.m_shared.m_maxStackSize >= stack;
	}

	public bool AddItem(GameObject prefab, int amount)
	{
		ItemDrop.ItemData itemData = prefab.GetComponent<ItemDrop>().m_itemData.Clone();
		itemData.m_dropPrefab = prefab;
		itemData.m_stack = Mathf.Min(amount, itemData.m_shared.m_maxStackSize);
		itemData.m_worldLevel = (byte)Game.m_worldLevel;
		ZLog.Log($"Adding item {prefab.name} x{itemData.m_stack}");
		return AddItem(itemData);
	}

	public bool AddItem(ItemDrop.ItemData item)
	{
		bool flag = true;
		bool cheatedStateChanged = item.m_cheated && !Achievements.IsCheatedAtAll();
		if (item.m_shared.m_maxStackSize > 1)
		{
			for (int i = 0; i < item.m_stack; i++)
			{
				ItemDrop.ItemData itemData = FindFreeStackItem(item.m_shared.m_name, item.m_quality, item.m_worldLevel);
				if (itemData != null)
				{
					itemData.m_stack++;
					if (item.m_cheated && !PlayerProfile.s_bypassCheatChecks)
					{
						itemData.m_cheated = true;
					}
					continue;
				}
				int stack = item.m_stack - i;
				item.m_stack = stack;
				Vector2i gridPos = FindEmptySlot(TopFirst(item));
				if (gridPos.x >= 0)
				{
					item.m_gridPos = gridPos;
					m_inventory.Add(item);
				}
				else
				{
					flag = false;
					ZLog.LogWarning($"Trying to add item to occupied slot {gridPos.x}, {gridPos.y}");
				}
				break;
			}
		}
		else
		{
			Vector2i gridPos2 = FindEmptySlot(TopFirst(item));
			if (gridPos2.x >= 0)
			{
				item.m_gridPos = gridPos2;
				m_inventory.Add(item);
			}
			else
			{
				flag = false;
				ZLog.LogWarning($"Trying to add item to occupied slot {gridPos2.x}, {gridPos2.y}");
			}
		}
		Changed(flag, cheatedStateChanged);
		return flag;
	}

	public bool AddItem(ItemDrop.ItemData item, Vector2i pos)
	{
		bool flag = true;
		bool cheatedStateChanged = item.m_cheated && !Achievements.IsCheatedAtAll();
		if (item.m_shared.m_maxStackSize > 1)
		{
			for (int i = 0; i < item.m_stack; i++)
			{
				ItemDrop.ItemData itemData = FindFreeStackItem(item.m_shared.m_name, item.m_quality, item.m_worldLevel);
				if (itemData != null)
				{
					itemData.m_stack++;
					continue;
				}
				int stack = item.m_stack - i;
				item.m_stack = stack;
				if (GetItemAt(pos.x, pos.y) == null)
				{
					item.m_gridPos = pos;
					m_inventory.Add(item);
				}
				else
				{
					flag = false;
					ZLog.LogWarning($"Trying to add item to occupied slot {pos.x}, {pos.y}");
				}
				break;
			}
		}
		else if (GetItemAt(pos.x, pos.y) == null)
		{
			item.m_gridPos = pos;
			m_inventory.Add(item);
		}
		else
		{
			flag = false;
			ZLog.LogWarning($"Trying to add item to occupied slot {pos.x}, {pos.y}");
		}
		Changed(flag, cheatedStateChanged);
		return flag;
	}

	private bool TopFirst(ItemDrop.ItemData item)
	{
		if (item.IsWeapon())
		{
			return true;
		}
		if (item.m_shared.m_itemType == ItemDrop.ItemData.ItemType.Tool || item.m_shared.m_itemType == ItemDrop.ItemData.ItemType.Shield || item.m_shared.m_itemType == ItemDrop.ItemData.ItemType.Utility || item.m_shared.m_itemType == ItemDrop.ItemData.ItemType.Misc || item.m_shared.m_itemType == ItemDrop.ItemData.ItemType.Trinket)
		{
			return true;
		}
		return false;
	}

	public void MoveAll(Inventory fromInventory)
	{
		List<ItemDrop.ItemData> list = new List<ItemDrop.ItemData>(fromInventory.GetAllItems());
		List<ItemDrop.ItemData> list2 = new List<ItemDrop.ItemData>();
		foreach (ItemDrop.ItemData item in list)
		{
			if (AddItem(item, item.m_stack, item.m_gridPos.x, item.m_gridPos.y))
			{
				fromInventory.RemoveItem(item);
			}
			else
			{
				list2.Add(item);
			}
		}
		foreach (ItemDrop.ItemData item2 in list2)
		{
			if (AddItem(item2))
			{
				fromInventory.RemoveItem(item2);
			}
		}
		Changed();
		fromInventory.Changed();
	}

	public int StackAll(Inventory fromInventory, bool message = false)
	{
		List<ItemDrop.ItemData> list = new List<ItemDrop.ItemData>(fromInventory.GetAllItems());
		int num = (message ? CountItems(null) : 0);
		foreach (ItemDrop.ItemData item in list)
		{
			if (ContainsItemByName(item.m_shared.m_name) && !Player.m_localPlayer.IsItemEquiped(item) && AddItem(item))
			{
				fromInventory.RemoveItem(item);
			}
		}
		int num2 = CountItems(null) - num;
		if (message)
		{
			if (num2 > 0)
			{
				Player.m_localPlayer.Message(MessageHud.MessageType.Center, "$msg_stackall " + num2);
			}
			else
			{
				Player.m_localPlayer.Message(MessageHud.MessageType.Center, "$msg_stackall_none");
			}
		}
		Changed();
		fromInventory.Changed();
		Game.instance.IncrementPlayerStat(PlayerStatType.PlaceStacks);
		return num2;
	}

	public bool ContainsItemByName(string name)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_shared.m_name == name)
			{
				return true;
			}
		}
		return false;
	}

	public void MoveItemToThis(Inventory fromInventory, ItemDrop.ItemData item)
	{
		if (AddItem(item))
		{
			fromInventory.RemoveItem(item);
		}
		Changed();
		fromInventory.Changed();
	}

	public bool MoveItemToThis(Inventory fromInventory, ItemDrop.ItemData item, int amount, int x, int y)
	{
		bool result = AddItem(item, amount, x, y);
		if (item.m_stack == 0)
		{
			fromInventory.RemoveItem(item);
			return result;
		}
		fromInventory.Changed();
		return result;
	}

	public bool RemoveItem(int index)
	{
		if (index < 0 || index >= m_inventory.Count)
		{
			return false;
		}
		m_inventory.RemoveAt(index);
		Changed();
		return true;
	}

	public bool ContainsItem(ItemDrop.ItemData item)
	{
		return m_inventory.Contains(item);
	}

	public bool RemoveOneItem(ItemDrop.ItemData item)
	{
		if (!m_inventory.Contains(item))
		{
			return false;
		}
		if (item.m_stack > 1)
		{
			item.m_stack--;
			Changed();
		}
		else
		{
			m_inventory.Remove(item);
			Changed();
		}
		return true;
	}

	public bool RemoveItem(ItemDrop.ItemData item)
	{
		if (!m_inventory.Contains(item))
		{
			ZLog.Log("Item is not in this container");
			return false;
		}
		m_inventory.Remove(item);
		Changed();
		return true;
	}

	public bool RemoveItem(ItemDrop.ItemData item, int amount)
	{
		amount = Mathf.Min(item.m_stack, amount);
		if (amount == item.m_stack)
		{
			return RemoveItem(item);
		}
		if (!m_inventory.Contains(item))
		{
			return false;
		}
		item.m_stack -= amount;
		Changed();
		return true;
	}

	public void RemoveItem(string name, int amount, int itemQuality = -1, bool worldLevelBased = true)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_shared.m_name == name && (itemQuality < 0 || item.m_quality == itemQuality) && (!worldLevelBased || item.m_worldLevel >= Game.m_worldLevel))
			{
				int num = Mathf.Min(item.m_stack, amount);
				item.m_stack -= num;
				amount -= num;
				if (amount <= 0)
				{
					break;
				}
			}
		}
		m_inventory.RemoveAll((ItemDrop.ItemData x) => x.m_stack <= 0);
		Changed();
	}

	public bool HaveItem(string name, bool matchWorldLevel = true)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_shared.m_name == name && (!matchWorldLevel || item.m_worldLevel >= Game.m_worldLevel))
			{
				return true;
			}
		}
		return false;
	}

	public bool HaveItem(ItemDrop.ItemData.ItemType type, bool matchWorldLevel = true)
	{
		return m_inventory.Any((ItemDrop.ItemData i) => i.m_shared.m_itemType == type && (!matchWorldLevel || i.m_worldLevel >= Game.m_worldLevel));
	}

	public void GetAllPieceTables(List<PieceTable> tables)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_shared.m_buildPieces != null && !tables.Contains(item.m_shared.m_buildPieces))
			{
				tables.Add(item.m_shared.m_buildPieces);
			}
		}
	}

	public int CountItems(string name, int quality = -1, bool matchWorldLevel = true)
	{
		int num = 0;
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if ((name == null || item.m_shared.m_name == name) && (quality < 0 || quality == item.m_quality) && (!matchWorldLevel || item.m_worldLevel >= Game.m_worldLevel))
			{
				num += item.m_stack;
			}
		}
		return num;
	}

	public bool ItemCheated(Piece.Requirement[] resources, int quality = -1, bool matchWorldLevel = true)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			foreach (Piece.Requirement requirement in resources)
			{
				if (item.m_shared.m_name == requirement.m_resItem.m_itemData.m_shared.m_name && (quality < 0 || quality == item.m_quality) && (!matchWorldLevel || item.m_worldLevel >= Game.m_worldLevel) && item.m_cheated)
				{
					return true;
				}
			}
		}
		return false;
	}

	public bool ItemCheated(string itemName, int quality = -1, bool matchWorldLevel = true)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_shared.m_name == itemName && (quality < 0 || quality == item.m_quality) && (!matchWorldLevel || item.m_worldLevel >= Game.m_worldLevel) && item.m_cheated)
			{
				return true;
			}
		}
		return false;
	}

	public int CountItemsByName(string[] names, int quality = -1, bool matchWorldLevel = true, bool stacksOnly = false)
	{
		int num = 0;
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if ((names == null || names.Contains(item.m_shared.m_name)) && (quality < 0 || quality == item.m_quality) && (!matchWorldLevel || item.m_worldLevel >= Game.m_worldLevel))
			{
				num += (stacksOnly ? 1 : item.m_stack);
			}
		}
		return num;
	}

	public int CountItemsByType(ItemDrop.ItemData.ItemType type, int quality = -1, bool matchWorldLevel = true, bool stacksOnly = false)
	{
		int num = 0;
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if ((type == ItemDrop.ItemData.ItemType.None || item.m_shared.m_itemType == type) && (quality < 0 || quality == item.m_quality) && (!matchWorldLevel || item.m_worldLevel >= Game.m_worldLevel))
			{
				num += (stacksOnly ? 1 : item.m_stack);
			}
		}
		return num;
	}

	public int CountItemsByType(ItemDrop.ItemData.ItemType[] types, int quality = -1, bool matchWorldLevel = true, bool stacksOnly = false)
	{
		int num = 0;
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if ((item.m_shared.m_itemType == ItemDrop.ItemData.ItemType.None || types.Contains(item.m_shared.m_itemType)) && (quality < 0 || quality == item.m_quality) && (!matchWorldLevel || item.m_worldLevel >= Game.m_worldLevel))
			{
				num += (stacksOnly ? 1 : item.m_stack);
			}
		}
		return num;
	}

	public ItemDrop.ItemData GetItem(int index)
	{
		return m_inventory[index];
	}

	public ItemDrop.ItemData GetItem(string name, int quality = -1, bool isPrefabName = false)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (((isPrefabName && item.m_dropPrefab.name == name) || (!isPrefabName && item.m_shared.m_name == name)) && (quality < 0 || quality == item.m_quality) && item.m_worldLevel >= Game.m_worldLevel)
			{
				return item;
			}
		}
		return null;
	}

	public ItemDrop.ItemData GetAmmoItem(string ammoName, string matchPrefabName = null)
	{
		int num = 0;
		ItemDrop.ItemData itemData = null;
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if ((item.m_shared.m_itemType == ItemDrop.ItemData.ItemType.Ammo || item.m_shared.m_itemType == ItemDrop.ItemData.ItemType.AmmoNonEquipable || item.m_shared.m_itemType == ItemDrop.ItemData.ItemType.Consumable) && item.m_shared.m_ammoType == ammoName && (matchPrefabName == null || item.m_dropPrefab.name == matchPrefabName))
			{
				int num2 = item.m_gridPos.y * m_width + item.m_gridPos.x;
				if (num2 < num || itemData == null)
				{
					num = num2;
					itemData = item;
				}
			}
		}
		return itemData;
	}

	public int FindFreeStackSpace(string name, float worldLevel)
	{
		int num = 0;
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_shared.m_name == name && item.m_stack < item.m_shared.m_maxStackSize && (float)item.m_worldLevel == worldLevel)
			{
				num += item.m_shared.m_maxStackSize - item.m_stack;
			}
		}
		return num;
	}

	private ItemDrop.ItemData FindFreeStackItem(string name, int quality, float worldLevel)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_shared.m_name == name && item.m_quality == quality && item.m_stack < item.m_shared.m_maxStackSize && (float)item.m_worldLevel == worldLevel)
			{
				return item;
			}
		}
		return null;
	}

	public int NrOfItems()
	{
		return m_inventory.Count;
	}

	public int NrOfItemsIncludingStacks()
	{
		int num = 0;
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			num += item.m_stack;
		}
		return num;
	}

	public float SlotsUsedPercentage()
	{
		return (float)m_inventory.Count / (float)(m_width * m_height) * 100f;
	}

	public void Print()
	{
		for (int i = 0; i < m_inventory.Count; i++)
		{
			ItemDrop.ItemData itemData = m_inventory[i];
			ZLog.Log(i + ": " + itemData.m_shared.m_name + "  " + itemData.m_stack + " / " + itemData.m_shared.m_maxStackSize);
		}
	}

	public int GetEmptySlots()
	{
		return m_height * m_width - m_inventory.Count;
	}

	public bool HaveEmptySlot()
	{
		return m_inventory.Count < m_width * m_height;
	}

	private Vector2i FindEmptySlot(bool topFirst)
	{
		if (topFirst)
		{
			for (int i = 0; i < m_height; i++)
			{
				for (int j = 0; j < m_width; j++)
				{
					if (GetItemAt(j, i) == null)
					{
						return new Vector2i(j, i);
					}
				}
			}
		}
		else
		{
			for (int num = m_height - 1; num >= 0; num--)
			{
				for (int k = 0; k < m_width; k++)
				{
					if (GetItemAt(k, num) == null)
					{
						return new Vector2i(k, num);
					}
				}
			}
		}
		return new Vector2i(-1, -1);
	}

	public ItemDrop.ItemData GetOtherItemAt(int x, int y, ItemDrop.ItemData oldItem)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item != oldItem && item.m_gridPos.x == x && item.m_gridPos.y == y)
			{
				return item;
			}
		}
		return null;
	}

	public ItemDrop.ItemData GetItemAt(int x, int y)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_gridPos.x == x && item.m_gridPos.y == y)
			{
				return item;
			}
		}
		return null;
	}

	public List<ItemDrop.ItemData> GetEquippedItems()
	{
		List<ItemDrop.ItemData> list = new List<ItemDrop.ItemData>();
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_equipped)
			{
				list.Add(item);
			}
		}
		return list;
	}

	public void GetWornItems(List<ItemDrop.ItemData> worn)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_shared.m_useDurability && item.m_durability < item.GetMaxDurability())
			{
				worn.Add(item);
			}
		}
	}

	public void GetValuableItems(List<ItemDrop.ItemData> items)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_shared.m_value > 0)
			{
				items.Add(item);
			}
		}
	}

	public List<ItemDrop.ItemData> GetAllItems()
	{
		return m_inventory;
	}

	public List<ItemDrop.ItemData> GetAllItemsSortedByName()
	{
		return m_inventory.OrderBy((ItemDrop.ItemData item) => item.m_shared.m_name).ToList();
	}

	public List<ItemDrop.ItemData> GetAllItemsInGridOrder()
	{
		return (from i in m_inventory
			orderby i.m_gridPos.y, i.m_gridPos.x
			select i).ToList();
	}

	public List<ItemDrop.ItemData> GetAllItemsOfType(ItemDrop.ItemData.ItemType type, bool sortByGridOrder = false)
	{
		if (sortByGridOrder)
		{
			return (from i in m_inventory
				where i.m_shared.m_itemType == type
				orderby i.m_gridPos.y, i.m_gridPos.x
				select i).ToList();
		}
		return m_inventory.Where((ItemDrop.ItemData i) => i.m_shared.m_itemType == type).ToList();
	}

	public List<ItemDrop.ItemData> GetAllItemsOfType(ItemDrop.ItemData.ItemType[] types, bool sortByGridOrder = false)
	{
		if (sortByGridOrder)
		{
			return (from i in m_inventory
				where types.Contains(i.m_shared.m_itemType)
				orderby i.m_gridPos.y, i.m_gridPos.x
				select i).ToList();
		}
		return m_inventory.Where((ItemDrop.ItemData i) => types.Contains(i.m_shared.m_itemType)).ToList();
	}

	public void GetAllItems(string name, List<ItemDrop.ItemData> items)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_shared.m_name == name && item.m_worldLevel >= Game.m_worldLevel)
			{
				items.Add(item);
			}
		}
	}

	public void GetAllItems(ItemDrop.ItemData.ItemType type, List<ItemDrop.ItemData> items)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_shared.m_itemType == type)
			{
				items.Add(item);
			}
		}
	}

	public bool CheatedDamagingItemEquipped()
	{
		foreach (ItemDrop.ItemData equippedItem in GetEquippedItems())
		{
			if (equippedItem.m_cheated && equippedItem.GetDamage().GetTotalDamage() > 0f)
			{
				return true;
			}
		}
		return false;
	}

	public bool AnyCheatedItem()
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_cheated)
			{
				return true;
			}
		}
		return false;
	}

	public int GetWidth()
	{
		return m_width;
	}

	public int GetHeight()
	{
		return m_height;
	}

	public void SetHeight(int height)
	{
		m_height = height;
	}

	public string GetName()
	{
		return m_name;
	}

	public Sprite GetBkg()
	{
		return m_bkg;
	}

	public void OldSave(ZPackage pkg)
	{
		pkg.Write(106);
		pkg.Write(m_inventory.Count);
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_dropPrefab == null)
			{
				ZLog.Log("Item missing prefab " + item.m_shared.m_name);
				pkg.Write("");
			}
			else
			{
				pkg.Write(item.m_dropPrefab.name);
			}
			pkg.Write(item.m_stack);
			pkg.Write(item.m_durability);
			pkg.Write(item.m_gridPos);
			pkg.Write(item.m_equipped);
			pkg.Write(item.m_quality);
			pkg.Write(item.m_variant);
			pkg.Write(item.m_crafterID);
			pkg.Write(item.m_crafterName);
			pkg.Write(item.m_customData.Count);
			foreach (KeyValuePair<string, string> customDatum in item.m_customData)
			{
				pkg.Write(customDatum.Key);
				pkg.Write(customDatum.Value);
			}
			pkg.Write(item.m_worldLevel);
			pkg.Write(item.m_pickedUp);
			pkg.Write(item.m_cheated);
		}
	}

	public void Save(ZPackage pkg)
	{
		pkg.Write(109);
		pkg.Write((ushort)m_inventory.Count);
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			item.Save(pkg);
		}
	}

	public void Load(ZPackage pkg, bool _)
	{
		m_inventory.Clear();
		Version.Item item = (Version.Item)pkg.ReadInt();
		if (item >= Version.Item.Smaller)
		{
			int num = pkg.ReadUShort();
			for (int i = 0; i < num; i++)
			{
				var (num2, itemData) = ItemDrop.ItemData.Load(pkg, item);
				if (num2 != 0)
				{
					AddItem(num2, itemData);
				}
			}
		}
		else
		{
			LoadOld(pkg, item);
		}
		Changed();
	}

	public void Load(ZPackage pkg)
	{
		m_inventory.Clear();
		Version.Item item = (Version.Item)pkg.ReadInt();
		if (item >= Version.Item.Smaller)
		{
			int num = pkg.ReadUShort();
			for (int i = 0; i < num; i++)
			{
				var (num2, itemData) = ItemDrop.ItemData.Load(pkg, item);
				if (num2 != 0)
				{
					AddItem(num2, itemData);
				}
			}
		}
		else
		{
			LoadOld(pkg, item);
		}
		Changed();
	}

	private void LoadOld(ZPackage pkg, Version.Item version)
	{
		int num = pkg.ReadInt();
		for (int i = 0; i < num; i++)
		{
			string text = pkg.ReadString();
			int stack = pkg.ReadInt();
			float durability = pkg.ReadSingle();
			Vector2i pos = pkg.ReadVector2i();
			bool equipped = pkg.ReadBool();
			int quality = 1;
			if (version >= Version.Item.Quality)
			{
				quality = pkg.ReadInt();
			}
			int variant = 0;
			if (version >= Version.Item.Variant)
			{
				variant = pkg.ReadInt();
			}
			long crafterID = 0L;
			string crafterName = "";
			if (version >= Version.Item.CrafterID)
			{
				crafterID = pkg.ReadLong();
				crafterName = pkg.ReadString();
			}
			Dictionary<string, string> dictionary = new Dictionary<string, string>();
			if (version >= Version.Item.CustomData)
			{
				int num2 = pkg.ReadInt();
				for (int j = 0; j < num2; j++)
				{
					string key = pkg.ReadString();
					string value = pkg.ReadString();
					dictionary[key] = value;
				}
			}
			int worldLevel = 0;
			if (version >= Version.Item.WorldLevel)
			{
				worldLevel = pkg.ReadInt();
			}
			bool pickedUp = false;
			if (version >= Version.Item.PickedUp)
			{
				pickedUp = pkg.ReadBool();
			}
			bool cheated = false;
			if (version == Version.Item.AbandonedDN || version >= Version.Item.ChunksNCheats)
			{
				cheated = pkg.ReadBool();
			}
			if (text != "")
			{
				AddItem(text, stack, durability, pos, equipped, quality, variant, crafterID, crafterName, dictionary, worldLevel, pickedUp, cheated);
			}
		}
	}

	public ItemDrop.ItemData AddItem(string name, int stack, int quality, int variant, long crafterID, string crafterName, bool cheated, bool pickedUp = false)
	{
		return AddItem(name, stack, quality, variant, crafterID, crafterName, new Vector2i(-1, -1), cheated, pickedUp);
	}

	private bool AddItem(int prefabHash, ItemDrop.ItemData itemData, bool skipValidPositionCheck = true)
	{
		return AddItem(prefabHash, itemData.m_stack, itemData.m_durability, itemData.m_gridPos, itemData.m_equipped, itemData.m_quality, itemData.m_variant, itemData.m_crafterID, itemData.m_crafterName, itemData.m_customData, itemData.m_worldLevel, itemData.m_pickedUp, itemData.m_cheated, skipValidPositionCheck);
	}

	public ItemDrop.ItemData AddItem(string name, int stack, int quality, int variant, long crafterID, string crafterName, Vector2i position, bool cheated, bool pickedUp = false, bool dropIfFullInv = true)
	{
		GameObject itemPrefab = ObjectDB.instance.GetItemPrefab(name);
		if (itemPrefab == null)
		{
			ZLog.Log("Failed to find item prefab " + name);
			return null;
		}
		ItemDrop component = itemPrefab.GetComponent<ItemDrop>();
		if (component == null)
		{
			ZLog.Log("Invalid item " + name);
			return null;
		}
		if (component.m_itemData.m_shared.m_maxStackSize <= 1 && FindEmptySlot(TopFirst(component.m_itemData)).x == -1)
		{
			return null;
		}
		ItemDrop.ItemData result = null;
		int num = stack;
		while (num > 0)
		{
			ZNetView.m_forceDisableInit = true;
			GameObject gameObject = UnityEngine.Object.Instantiate(itemPrefab);
			ZNetView.m_forceDisableInit = false;
			ItemDrop component2 = gameObject.GetComponent<ItemDrop>();
			if (component2 == null)
			{
				ZLog.Log("Missing itemdrop in " + name);
				UnityEngine.Object.Destroy(gameObject);
				return null;
			}
			int num2 = Mathf.Min(num, component2.m_itemData.m_shared.m_maxStackSize);
			num -= num2;
			component2.m_itemData.m_stack = num2;
			component2.SetQuality(quality);
			component2.m_itemData.m_variant = variant;
			component2.m_itemData.m_durability = component2.m_itemData.GetMaxDurability();
			component2.m_itemData.m_crafterID = crafterID;
			component2.m_itemData.m_crafterName = crafterName;
			component2.m_itemData.m_worldLevel = (byte)Game.m_worldLevel;
			component2.m_itemData.m_pickedUp = pickedUp;
			component2.m_itemData.m_cheated = cheated;
			if (!((position.x >= 0 && position.y >= 0 && position.x < m_width && position.y < m_height) ? AddItem(component2.m_itemData, position) : AddItem(component2.m_itemData)))
			{
				if (!dropIfFullInv)
				{
					UnityEngine.Object.Destroy(gameObject);
					ZLog.LogError("Failed to add new item " + name + " in inventory, deleting!");
					return null;
				}
				UnityEngine.Object.Instantiate(itemPrefab, Player.m_localPlayer.transform.position + Vector3.up, Quaternion.identity);
				ZLog.LogWarning("Failed to add new item " + name + " in inventory, dropping!");
			}
			result = component2.m_itemData;
			UnityEngine.Object.Destroy(gameObject);
		}
		return result;
	}

	private bool AddTempItem(int prefabHash, int stack, float durability, Vector2i pos, bool equipped, int quality, int variant, long crafterID, string crafterName, Dictionary<string, string> customData, int worldLevel, bool pickedUp)
	{
		GameObject itemPrefab = ObjectDB.instance.GetItemPrefab(prefabHash);
		if (itemPrefab == null)
		{
			ZLog.Log("Failed to find item prefab " + prefabHash);
			return false;
		}
		ItemDrop.ItemData itemData = new ItemDrop.ItemData();
		itemData.m_stack = stack;
		itemData.m_durability = durability;
		itemData.m_equipped = equipped;
		itemData.m_quality = quality;
		itemData.m_variant = variant;
		itemData.m_crafterID = crafterID;
		itemData.m_crafterName = crafterName;
		itemData.m_customData = customData;
		itemData.m_worldLevel = (byte)worldLevel;
		itemData.m_pickedUp = pickedUp;
		itemData.m_dropPrefab = itemPrefab;
		AddItem(itemData, itemData.m_stack, pos.x, pos.y);
		return true;
	}

	private bool AddItem(string name, int stack, float durability, Vector2i pos, bool equipped, int quality, int variant, long crafterID, string crafterName, Dictionary<string, string> customData, int worldLevel, bool pickedUp, bool cheated, bool skipValidPositionCheck = false)
	{
		int prefabHash = ((!string.IsNullOrEmpty(name)) ? name.GetStableHashCode() : 0);
		return AddItem(prefabHash, stack, durability, pos, equipped, quality, variant, crafterID, crafterName, customData, worldLevel, pickedUp, cheated, skipValidPositionCheck);
	}

	private bool AddItem(int prefabHash, int stack, float durability, Vector2i pos, bool equipped, int quality, int variant, long crafterID, string crafterName, Dictionary<string, string> customData, int worldLevel, bool pickedUp, bool cheated, bool skipValidPositionCheck = false)
	{
		if (m_temoraryInventory)
		{
			return AddTempItem(prefabHash, stack, durability, pos, equipped, quality, variant, crafterID, crafterName, customData, worldLevel, pickedUp);
		}
		GameObject itemPrefab = ObjectDB.instance.GetItemPrefab(prefabHash);
		if (itemPrefab == null)
		{
			ZLog.Log("Failed to find item prefab " + prefabHash);
			return false;
		}
		ZNetView.m_forceDisableInit = true;
		GameObject gameObject = UnityEngine.Object.Instantiate(itemPrefab);
		ZNetView.m_forceDisableInit = false;
		ItemDrop component = gameObject.GetComponent<ItemDrop>();
		if (component == null)
		{
			ZLog.Log("Missing itemdrop in " + prefabHash);
			UnityEngine.Object.Destroy(gameObject);
			return false;
		}
		component.m_itemData.m_stack = Mathf.Min(stack, component.m_itemData.m_shared.m_maxStackSize);
		component.m_itemData.m_durability = durability;
		component.m_itemData.m_equipped = equipped;
		component.SetQuality(quality);
		component.m_itemData.m_variant = variant;
		component.m_itemData.m_crafterID = crafterID;
		component.m_itemData.m_crafterName = crafterName;
		component.m_itemData.m_customData = customData;
		component.m_itemData.m_worldLevel = (byte)worldLevel;
		component.m_itemData.m_pickedUp = pickedUp;
		component.m_itemData.m_cheated = cheated || component.m_itemData.GetDamage().GetTotalDamage() > 10000f;
		AddItem(component.m_itemData, component.m_itemData.m_stack, pos.x, pos.y, skipValidPositionCheck);
		UnityEngine.Object.Destroy(gameObject);
		return true;
	}

	public void MoveInventoryToGrave(Inventory original)
	{
		m_inventory.Clear();
		m_width = original.m_width;
		m_height = original.m_height;
		foreach (ItemDrop.ItemData item in original.m_inventory)
		{
			if (!item.m_shared.m_questItem && !item.m_equipped)
			{
				m_inventory.Add(item);
			}
		}
		original.m_inventory.RemoveAll((ItemDrop.ItemData x) => !x.m_shared.m_questItem && !x.m_equipped);
		original.Changed();
		Changed();
	}

	private void Changed(bool success = false, bool cheatedStateChanged = false)
	{
		if (!m_temoraryInventory)
		{
			UpdateTotalWeight();
			if (success & cheatedStateChanged)
			{
				MessageHud.instance.ShowMessage(MessageHud.MessageType.Center, "$achievements_picked_up_cheated_item");
				m_cheatedPopup = true;
			}
			else if (m_cheatedPopup && (object)Player.m_localPlayer != null && !Player.m_localPlayer.GetInventory().AnyCheatedItem())
			{
				MessageHud.instance.ShowMessage(MessageHud.MessageType.Center, "$achievements_dropped_cheated_item");
				m_cheatedPopup = false;
			}
			if (m_onChanged != null)
			{
				m_onChanged();
			}
		}
	}

	public void RemoveAll()
	{
		m_inventory.Clear();
		Changed();
	}

	public void RemoveUnequipped()
	{
		m_inventory.RemoveAll((ItemDrop.ItemData x) => !x.m_shared.m_questItem && !x.m_equipped);
		Changed();
	}

	private void UpdateTotalWeight()
	{
		m_totalWeight = 0f;
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			m_totalWeight += item.GetWeight();
		}
	}

	public float GetTotalWeight()
	{
		return m_totalWeight;
	}

	public void GetBoundItems(List<ItemDrop.ItemData> bound)
	{
		bound.Clear();
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_gridPos.y == 0)
			{
				bound.Add(item);
			}
		}
	}

	public List<ItemDrop.ItemData> GetHotbar(bool includeEmpty = false)
	{
		if (!includeEmpty)
		{
			return (from itemData in m_inventory
				where itemData.m_gridPos.y == 0
				orderby itemData.m_gridPos.x
				select itemData).ToList();
		}
		List<ItemDrop.ItemData> list = new List<ItemDrop.ItemData>();
		List<ItemDrop.ItemData> source = m_inventory.Where((ItemDrop.ItemData itemData) => itemData.m_gridPos.y == 0).ToList();
		int i;
		for (i = 0; i < 8; i++)
		{
			list.Add(source.FirstOrDefault((ItemDrop.ItemData item) => item.m_gridPos.x == i));
		}
		return list;
	}

	public bool IsTeleportable(bool allowAllItems)
	{
		foreach (ItemDrop.ItemData item in m_inventory)
		{
			if (item.m_shared.m_toolTier >= 1000)
			{
				return false;
			}
		}
		if (allowAllItems || ZoneSystem.instance.GetGlobalKey(GlobalKeys.TeleportAll))
		{
			return true;
		}
		foreach (ItemDrop.ItemData item2 in m_inventory)
		{
			if (!item2.m_shared.m_teleportable)
			{
				return false;
			}
		}
		return true;
	}
}
