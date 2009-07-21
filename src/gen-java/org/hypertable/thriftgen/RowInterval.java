/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
package org.hypertable.thriftgen;

import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import java.util.Set;
import java.util.HashSet;
import java.util.Collections;
import org.apache.log4j.Logger;

import org.apache.thrift.*;
import org.apache.thrift.meta_data.*;
import org.apache.thrift.protocol.*;

/**
 * Specifies a range of rows
 * 
 * <dl>
 *   <dt>start_row</dt>
 *   <dd>The row to start scan with. Must not contain nulls (0x00)</dd>
 * 
 *   <dt>start_inclusive</dt>
 *   <dd>Whether the start row is included in the result (default: true)</dd>
 * 
 *   <dt>end_row</dt>
 *   <dd>The row to end scan with. Must not contain nulls</dd>
 * 
 *   <dt>end_inclusive</dt>
 *   <dd>Whether the end row is included in the result (default: true)</dd>
 * </dl>
 */
public class RowInterval implements TBase, java.io.Serializable, Cloneable {
  private static final TStruct STRUCT_DESC = new TStruct("RowInterval");
  private static final TField START_ROW_FIELD_DESC = new TField("start_row", TType.STRING, (short)1);
  private static final TField START_INCLUSIVE_FIELD_DESC = new TField("start_inclusive", TType.BOOL, (short)2);
  private static final TField END_ROW_FIELD_DESC = new TField("end_row", TType.STRING, (short)3);
  private static final TField END_INCLUSIVE_FIELD_DESC = new TField("end_inclusive", TType.BOOL, (short)4);

  public String start_row;
  public static final int START_ROW = 1;
  public boolean start_inclusive;
  public static final int START_INCLUSIVE = 2;
  public String end_row;
  public static final int END_ROW = 3;
  public boolean end_inclusive;
  public static final int END_INCLUSIVE = 4;

  private final Isset __isset = new Isset();
  private static final class Isset implements java.io.Serializable {
    public boolean start_inclusive = false;
    public boolean end_inclusive = false;
  }

  public static final Map<Integer, FieldMetaData> metaDataMap = Collections.unmodifiableMap(new HashMap<Integer, FieldMetaData>() {{
    put(START_ROW, new FieldMetaData("start_row", TFieldRequirementType.OPTIONAL, 
        new FieldValueMetaData(TType.STRING)));
    put(START_INCLUSIVE, new FieldMetaData("start_inclusive", TFieldRequirementType.OPTIONAL, 
        new FieldValueMetaData(TType.BOOL)));
    put(END_ROW, new FieldMetaData("end_row", TFieldRequirementType.OPTIONAL, 
        new FieldValueMetaData(TType.STRING)));
    put(END_INCLUSIVE, new FieldMetaData("end_inclusive", TFieldRequirementType.OPTIONAL, 
        new FieldValueMetaData(TType.BOOL)));
  }});

  static {
    FieldMetaData.addStructMetaDataMap(RowInterval.class, metaDataMap);
  }

  public RowInterval() {
    this.start_inclusive = true;

    this.end_inclusive = true;

  }

  public RowInterval(
    String start_row,
    boolean start_inclusive,
    String end_row,
    boolean end_inclusive)
  {
    this();
    this.start_row = start_row;
    this.start_inclusive = start_inclusive;
    this.__isset.start_inclusive = true;
    this.end_row = end_row;
    this.end_inclusive = end_inclusive;
    this.__isset.end_inclusive = true;
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public RowInterval(RowInterval other) {
    if (other.isSetStart_row()) {
      this.start_row = other.start_row;
    }
    __isset.start_inclusive = other.__isset.start_inclusive;
    this.start_inclusive = other.start_inclusive;
    if (other.isSetEnd_row()) {
      this.end_row = other.end_row;
    }
    __isset.end_inclusive = other.__isset.end_inclusive;
    this.end_inclusive = other.end_inclusive;
  }

  @Override
  public RowInterval clone() {
    return new RowInterval(this);
  }

  public String getStart_row() {
    return this.start_row;
  }

  public void setStart_row(String start_row) {
    this.start_row = start_row;
  }

  public void unsetStart_row() {
    this.start_row = null;
  }

  // Returns true if field start_row is set (has been asigned a value) and false otherwise
  public boolean isSetStart_row() {
    return this.start_row != null;
  }

  public void setStart_rowIsSet(boolean value) {
    if (!value) {
      this.start_row = null;
    }
  }

  public boolean isStart_inclusive() {
    return this.start_inclusive;
  }

  public void setStart_inclusive(boolean start_inclusive) {
    this.start_inclusive = start_inclusive;
    this.__isset.start_inclusive = true;
  }

  public void unsetStart_inclusive() {
    this.__isset.start_inclusive = false;
  }

  // Returns true if field start_inclusive is set (has been asigned a value) and false otherwise
  public boolean isSetStart_inclusive() {
    return this.__isset.start_inclusive;
  }

  public void setStart_inclusiveIsSet(boolean value) {
    this.__isset.start_inclusive = value;
  }

  public String getEnd_row() {
    return this.end_row;
  }

  public void setEnd_row(String end_row) {
    this.end_row = end_row;
  }

  public void unsetEnd_row() {
    this.end_row = null;
  }

  // Returns true if field end_row is set (has been asigned a value) and false otherwise
  public boolean isSetEnd_row() {
    return this.end_row != null;
  }

  public void setEnd_rowIsSet(boolean value) {
    if (!value) {
      this.end_row = null;
    }
  }

  public boolean isEnd_inclusive() {
    return this.end_inclusive;
  }

  public void setEnd_inclusive(boolean end_inclusive) {
    this.end_inclusive = end_inclusive;
    this.__isset.end_inclusive = true;
  }

  public void unsetEnd_inclusive() {
    this.__isset.end_inclusive = false;
  }

  // Returns true if field end_inclusive is set (has been asigned a value) and false otherwise
  public boolean isSetEnd_inclusive() {
    return this.__isset.end_inclusive;
  }

  public void setEnd_inclusiveIsSet(boolean value) {
    this.__isset.end_inclusive = value;
  }

  public void setFieldValue(int fieldID, Object value) {
    switch (fieldID) {
    case START_ROW:
      if (value == null) {
        unsetStart_row();
      } else {
        setStart_row((String)value);
      }
      break;

    case START_INCLUSIVE:
      if (value == null) {
        unsetStart_inclusive();
      } else {
        setStart_inclusive((Boolean)value);
      }
      break;

    case END_ROW:
      if (value == null) {
        unsetEnd_row();
      } else {
        setEnd_row((String)value);
      }
      break;

    case END_INCLUSIVE:
      if (value == null) {
        unsetEnd_inclusive();
      } else {
        setEnd_inclusive((Boolean)value);
      }
      break;

    default:
      throw new IllegalArgumentException("Field " + fieldID + " doesn't exist!");
    }
  }

  public Object getFieldValue(int fieldID) {
    switch (fieldID) {
    case START_ROW:
      return getStart_row();

    case START_INCLUSIVE:
      return new Boolean(isStart_inclusive());

    case END_ROW:
      return getEnd_row();

    case END_INCLUSIVE:
      return new Boolean(isEnd_inclusive());

    default:
      throw new IllegalArgumentException("Field " + fieldID + " doesn't exist!");
    }
  }

  // Returns true if field corresponding to fieldID is set (has been asigned a value) and false otherwise
  public boolean isSet(int fieldID) {
    switch (fieldID) {
    case START_ROW:
      return isSetStart_row();
    case START_INCLUSIVE:
      return isSetStart_inclusive();
    case END_ROW:
      return isSetEnd_row();
    case END_INCLUSIVE:
      return isSetEnd_inclusive();
    default:
      throw new IllegalArgumentException("Field " + fieldID + " doesn't exist!");
    }
  }

  @Override
  public boolean equals(Object that) {
    if (that == null)
      return false;
    if (that instanceof RowInterval)
      return this.equals((RowInterval)that);
    return false;
  }

  public boolean equals(RowInterval that) {
    if (that == null)
      return false;

    boolean this_present_start_row = true && this.isSetStart_row();
    boolean that_present_start_row = true && that.isSetStart_row();
    if (this_present_start_row || that_present_start_row) {
      if (!(this_present_start_row && that_present_start_row))
        return false;
      if (!this.start_row.equals(that.start_row))
        return false;
    }

    boolean this_present_start_inclusive = true && this.isSetStart_inclusive();
    boolean that_present_start_inclusive = true && that.isSetStart_inclusive();
    if (this_present_start_inclusive || that_present_start_inclusive) {
      if (!(this_present_start_inclusive && that_present_start_inclusive))
        return false;
      if (this.start_inclusive != that.start_inclusive)
        return false;
    }

    boolean this_present_end_row = true && this.isSetEnd_row();
    boolean that_present_end_row = true && that.isSetEnd_row();
    if (this_present_end_row || that_present_end_row) {
      if (!(this_present_end_row && that_present_end_row))
        return false;
      if (!this.end_row.equals(that.end_row))
        return false;
    }

    boolean this_present_end_inclusive = true && this.isSetEnd_inclusive();
    boolean that_present_end_inclusive = true && that.isSetEnd_inclusive();
    if (this_present_end_inclusive || that_present_end_inclusive) {
      if (!(this_present_end_inclusive && that_present_end_inclusive))
        return false;
      if (this.end_inclusive != that.end_inclusive)
        return false;
    }

    return true;
  }

  @Override
  public int hashCode() {
    return 0;
  }

  public void read(TProtocol iprot) throws TException {
    TField field;
    iprot.readStructBegin();
    while (true)
    {
      field = iprot.readFieldBegin();
      if (field.type == TType.STOP) { 
        break;
      }
      switch (field.id)
      {
        case START_ROW:
          if (field.type == TType.STRING) {
            this.start_row = iprot.readString();
          } else { 
            TProtocolUtil.skip(iprot, field.type);
          }
          break;
        case START_INCLUSIVE:
          if (field.type == TType.BOOL) {
            this.start_inclusive = iprot.readBool();
            this.__isset.start_inclusive = true;
          } else { 
            TProtocolUtil.skip(iprot, field.type);
          }
          break;
        case END_ROW:
          if (field.type == TType.STRING) {
            this.end_row = iprot.readString();
          } else { 
            TProtocolUtil.skip(iprot, field.type);
          }
          break;
        case END_INCLUSIVE:
          if (field.type == TType.BOOL) {
            this.end_inclusive = iprot.readBool();
            this.__isset.end_inclusive = true;
          } else { 
            TProtocolUtil.skip(iprot, field.type);
          }
          break;
        default:
          TProtocolUtil.skip(iprot, field.type);
          break;
      }
      iprot.readFieldEnd();
    }
    iprot.readStructEnd();


    // check for required fields of primitive type, which can't be checked in the validate method
    validate();
  }

  public void write(TProtocol oprot) throws TException {
    validate();

    oprot.writeStructBegin(STRUCT_DESC);
    if (this.start_row != null) {
      oprot.writeFieldBegin(START_ROW_FIELD_DESC);
      oprot.writeString(this.start_row);
      oprot.writeFieldEnd();
    }
    oprot.writeFieldBegin(START_INCLUSIVE_FIELD_DESC);
    oprot.writeBool(this.start_inclusive);
    oprot.writeFieldEnd();
    if (this.end_row != null) {
      oprot.writeFieldBegin(END_ROW_FIELD_DESC);
      oprot.writeString(this.end_row);
      oprot.writeFieldEnd();
    }
    oprot.writeFieldBegin(END_INCLUSIVE_FIELD_DESC);
    oprot.writeBool(this.end_inclusive);
    oprot.writeFieldEnd();
    oprot.writeFieldStop();
    oprot.writeStructEnd();
  }

  @Override
  public String toString() {
    StringBuilder sb = new StringBuilder("RowInterval(");
    boolean first = true;

    if (isSetStart_row()) {
      sb.append("start_row:");
      if (this.start_row == null) {
        sb.append("null");
      } else {
        sb.append(this.start_row);
      }
      first = false;
    }
    if (isSetStart_inclusive()) {
      if (!first) sb.append(", ");
      sb.append("start_inclusive:");
      sb.append(this.start_inclusive);
      first = false;
    }
    if (isSetEnd_row()) {
      if (!first) sb.append(", ");
      sb.append("end_row:");
      if (this.end_row == null) {
        sb.append("null");
      } else {
        sb.append(this.end_row);
      }
      first = false;
    }
    if (isSetEnd_inclusive()) {
      if (!first) sb.append(", ");
      sb.append("end_inclusive:");
      sb.append(this.end_inclusive);
      first = false;
    }
    sb.append(")");
    return sb.toString();
  }

  public void validate() throws TException {
    // check for required fields
    // check that fields of type enum have valid values
  }

}

