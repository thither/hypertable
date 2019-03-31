<?php
namespace Hypertable_ThriftGen;

/**
 * Autogenerated by Thrift Compiler (0.12.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
use Thrift\Base\TBase;
use Thrift\Type\TType;
use Thrift\Type\TMessageType;
use Thrift\Exception\TException;
use Thrift\Exception\TProtocolException;
use Thrift\Protocol\TProtocol;
use Thrift\Protocol\TBinaryProtocolAccelerated;
use Thrift\Exception\TApplicationException;

/**
 * Describes a ColumnFamily
 * <dl>
 *   <dt>name</dt>
 *   <dd>Name of the column family</dd>
 * 
 *   <dt>ag</dt>
 *   <dd>Name of the access group for this CF</dd>
 * 
 *   <dt>max_versions</dt>
 *   <dd>Max versions of the same cell to be stored</dd>
 * 
 *   <dt>ttl</dt>
 *   <dd>Time to live for cells in the CF (ie delete cells older than this time)</dd>
 * </dl>
 */
class ColumnFamilySpec
{
    static public $isValidate = false;

    static public $_TSPEC = array(
        1 => array(
            'var' => 'name',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
        2 => array(
            'var' => 'access_group',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
        3 => array(
            'var' => 'deleted',
            'isRequired' => false,
            'type' => TType::BOOL,
        ),
        4 => array(
            'var' => 'generation',
            'isRequired' => false,
            'type' => TType::I64,
        ),
        5 => array(
            'var' => 'id',
            'isRequired' => false,
            'type' => TType::I32,
        ),
        6 => array(
            'var' => 'value_index',
            'isRequired' => false,
            'type' => TType::BOOL,
        ),
        7 => array(
            'var' => 'qualifier_index',
            'isRequired' => false,
            'type' => TType::BOOL,
        ),
        8 => array(
            'var' => 'options',
            'isRequired' => false,
            'type' => TType::STRUCT,
            'class' => '\Hypertable_ThriftGen\ColumnFamilyOptions',
        ),
    );

    /**
     * @var string
     */
    public $name = null;
    /**
     * @var string
     */
    public $access_group = null;
    /**
     * @var bool
     */
    public $deleted = null;
    /**
     * @var int
     */
    public $generation = null;
    /**
     * @var int
     */
    public $id = null;
    /**
     * @var bool
     */
    public $value_index = null;
    /**
     * @var bool
     */
    public $qualifier_index = null;
    /**
     * @var \Hypertable_ThriftGen\ColumnFamilyOptions
     */
    public $options = null;

    public function __construct($vals = null)
    {
        if (is_array($vals)) {
            if (isset($vals['name'])) {
                $this->name = $vals['name'];
            }
            if (isset($vals['access_group'])) {
                $this->access_group = $vals['access_group'];
            }
            if (isset($vals['deleted'])) {
                $this->deleted = $vals['deleted'];
            }
            if (isset($vals['generation'])) {
                $this->generation = $vals['generation'];
            }
            if (isset($vals['id'])) {
                $this->id = $vals['id'];
            }
            if (isset($vals['value_index'])) {
                $this->value_index = $vals['value_index'];
            }
            if (isset($vals['qualifier_index'])) {
                $this->qualifier_index = $vals['qualifier_index'];
            }
            if (isset($vals['options'])) {
                $this->options = $vals['options'];
            }
        }
    }

    public function getName()
    {
        return 'ColumnFamilySpec';
    }


    public function read($input)
    {
        $xfer = 0;
        $fname = null;
        $ftype = 0;
        $fid = 0;
        $xfer += $input->readStructBegin($fname);
        while (true) {
            $xfer += $input->readFieldBegin($fname, $ftype, $fid);
            if ($ftype == TType::STOP) {
                break;
            }
            switch ($fid) {
                case 1:
                    if ($ftype == TType::STRING) {
                        $xfer += $input->readString($this->name);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 2:
                    if ($ftype == TType::STRING) {
                        $xfer += $input->readString($this->access_group);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 3:
                    if ($ftype == TType::BOOL) {
                        $xfer += $input->readBool($this->deleted);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 4:
                    if ($ftype == TType::I64) {
                        $xfer += $input->readI64($this->generation);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 5:
                    if ($ftype == TType::I32) {
                        $xfer += $input->readI32($this->id);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 6:
                    if ($ftype == TType::BOOL) {
                        $xfer += $input->readBool($this->value_index);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 7:
                    if ($ftype == TType::BOOL) {
                        $xfer += $input->readBool($this->qualifier_index);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 8:
                    if ($ftype == TType::STRUCT) {
                        $this->options = new \Hypertable_ThriftGen\ColumnFamilyOptions();
                        $xfer += $this->options->read($input);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                default:
                    $xfer += $input->skip($ftype);
                    break;
            }
            $xfer += $input->readFieldEnd();
        }
        $xfer += $input->readStructEnd();
        return $xfer;
    }

    public function write($output)
    {
        $xfer = 0;
        $xfer += $output->writeStructBegin('ColumnFamilySpec');
        if ($this->name !== null) {
            $xfer += $output->writeFieldBegin('name', TType::STRING, 1);
            $xfer += $output->writeString($this->name);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->access_group !== null) {
            $xfer += $output->writeFieldBegin('access_group', TType::STRING, 2);
            $xfer += $output->writeString($this->access_group);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->deleted !== null) {
            $xfer += $output->writeFieldBegin('deleted', TType::BOOL, 3);
            $xfer += $output->writeBool($this->deleted);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->generation !== null) {
            $xfer += $output->writeFieldBegin('generation', TType::I64, 4);
            $xfer += $output->writeI64($this->generation);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->id !== null) {
            $xfer += $output->writeFieldBegin('id', TType::I32, 5);
            $xfer += $output->writeI32($this->id);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->value_index !== null) {
            $xfer += $output->writeFieldBegin('value_index', TType::BOOL, 6);
            $xfer += $output->writeBool($this->value_index);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->qualifier_index !== null) {
            $xfer += $output->writeFieldBegin('qualifier_index', TType::BOOL, 7);
            $xfer += $output->writeBool($this->qualifier_index);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->options !== null) {
            if (!is_object($this->options)) {
                throw new TProtocolException('Bad type in structure.', TProtocolException::INVALID_DATA);
            }
            $xfer += $output->writeFieldBegin('options', TType::STRUCT, 8);
            $xfer += $this->options->write($output);
            $xfer += $output->writeFieldEnd();
        }
        $xfer += $output->writeFieldStop();
        $xfer += $output->writeStructEnd();
        return $xfer;
    }
}