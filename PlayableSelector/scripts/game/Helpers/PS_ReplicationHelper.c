class PS_ReplicationHelper
{
	// I want template functions, this seems ugly :(
	static void WriteMapIntInt(ScriptBitWriter writer, map<int, int> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteInt(mapToWrite.GetElement(i));
		}
	}
	static void ReadMapIntInt(ScriptBitReader reader, map<int, int> mapToWrite)
	{
		int count;
		reader.ReadInt(count);
		for (int i = 0; i < count; i++)
		{
			int key;
			int value;
			reader.ReadInt(key);
			reader.ReadInt(value);

			mapToWrite.Insert(key, value);
		}
	}

	static void WriteMapRplIdInt(ScriptBitWriter writer, map<RplId, int> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteInt(mapToWrite.GetElement(i));
		}
	}
	static void ReadMapRplIdInt(ScriptBitReader reader, map<RplId, int> mapToWrite)
	{
		int count;
		reader.ReadInt(count);
		for (int i = 0; i < count; i++)
		{
			RplId key;
			int value;
			reader.ReadInt(key);
			reader.ReadInt(value);

			mapToWrite.Insert(key, value);
		}
	}

	static void WriteMapIntRplId(ScriptBitWriter writer, map<int, RplId> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteInt(mapToWrite.GetElement(i));
		}
	}
	static void ReadMapIntRplId(ScriptBitReader reader, map<int, RplId> mapToWrite)
	{
		int count;
		reader.ReadInt(count);
		for (int i = 0; i < count; i++)
		{
			int key;
			RplId value;
			reader.ReadInt(key);
			reader.ReadInt(value);

			mapToWrite.Insert(key, value);
		}
	}

	static void WriteMapIntBool(ScriptBitWriter writer, map<int, bool> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteBool(mapToWrite.GetElement(i));
		}
	}
	static void ReadMapIntBool(ScriptBitReader reader, map<int, bool> mapToWrite)
	{
		int count;
		reader.ReadInt(count);
		for (int i = 0; i < count; i++)
		{
			int key;
			bool value;
			reader.ReadInt(key);
			reader.ReadBool(value);

			mapToWrite.Insert(key, value);
		}
	}

	static void WriteMapIntString(ScriptBitWriter writer, map<int, string> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteString(mapToWrite.GetElement(i));
		}
	}
	static void ReadMapIntString(ScriptBitReader reader, map<int, string> mapToWrite)
	{
		int count;
		reader.ReadInt(count);
		for (int i = 0; i < count; i++)
		{
			int key;
			string value;
			reader.ReadInt(key);
			reader.ReadString(value);

			mapToWrite.Insert(key, value);
		}
	}

	static void WriteMapIntFactionKey(ScriptBitWriter writer, map<int, FactionKey> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteString(mapToWrite.GetElement(i));
		}
	}
	static void ReadMapIntFactionKey(ScriptBitReader reader, map<int, FactionKey> mapToWrite)
	{
		int count;
		reader.ReadInt(count);
		for (int i = 0; i < count; i++)
		{
			int key;
			FactionKey value;
			reader.ReadInt(key);
			reader.ReadString(value);

			mapToWrite.Insert(key, value);
		}
	}

	static void WriteMapFactionKeyInt(ScriptBitWriter writer, map<FactionKey, int> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteString(mapToWrite.GetKey(i));
			writer.WriteInt(mapToWrite.GetElement(i));
		}
	}
	static void ReadMapFactionKeyInt(ScriptBitReader reader, map<FactionKey, int> mapToWrite)
	{
		int count;
		reader.ReadInt(count);
		for (int i = 0; i < count; i++)
		{
			FactionKey key;
			int value;
			reader.ReadString(key);
			reader.ReadInt(value);

			mapToWrite.Insert(key, value);
		}
	}

	static void WriteMapRplIdString(ScriptBitWriter writer, map<RplId, string> mapToWrite)
	{
		int count = mapToWrite.Count();
		writer.WriteInt(count);
		for (int i = 0; i < count; i++)
		{
			writer.WriteInt(mapToWrite.GetKey(i));
			writer.WriteString(mapToWrite.GetElement(i));
		}
	}
	static void ReadMapRplIdString(ScriptBitReader reader, map<RplId, string> mapToWrite)
	{
		int count;
		reader.ReadInt(count);
		for (int i = 0; i < count; i++)
		{
			RplId key;
			string value;
			reader.ReadInt(key);
			reader.ReadString(value);

			mapToWrite.Insert(key, value);
		}
	}
}
