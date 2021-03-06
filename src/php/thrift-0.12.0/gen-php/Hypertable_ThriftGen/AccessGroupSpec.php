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
 * Describes an AccessGroup
 * <dl>
 *   <dt>name</dt>
 *   <dd>Name of the access group</dd>
 * 
 *   <dt>in_memory</dt>
 *   <dd>Is this access group in memory</dd>
 * 
 *   <dt>replication</dt>
 *   <dd>Replication factor for this AG</dd>
 * 
 *   <dt>blocksize</dt>
 *   <dd>Specifies blocksize for this AG</dd>
 * 
 *   <dt>compressor</dt>
 *   <dd>Specifies compressor for this AG</dd>
 * 
 *   <dt>bloom_filter</dt>
 *   <dd>Specifies bloom filter type</dd>
 * 
 *   <dt>columns</dt>
 *   <dd>Specifies list of column families in this AG</dd>
 * </dl>
 */
class AccessGroupSpec
{
    static public $isValidate = false;

    static public $_TSPEC = array(
        1 => array(
            'var' => 'name',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
        2 => array(
            'var' => 'generation',
            'isRequired' => false,
            'type' => TType::I64,
        ),
        3 => array(
            'var' => 'options',
            'isRequired' => false,
            'type' => TType::STRUCT,
            'class' => '\Hypertable_ThriftGen\AccessGroupOptions',
        ),
        4 => array(
            'var' => 'defaults',
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
     * @var int
     */
    public $generation = null;
    /**
     * @var \Hypertable_ThriftGen\AccessGroupOptions
     */
    public $options = null;
    /**
     * @var \Hypertable_ThriftGen\ColumnFamilyOptions
     */
    public $defaults = null;

    public function __construct($vals = null)
    {
        if (is_array($vals)) {
            if (isset($vals['name'])) {
                $this->name = $vals['name'];
            }
            if (isset($vals['generation'])) {
                $this->generation = $vals['generation'];
            }
            if (isset($vals['options'])) {
                $this->options = $vals['options'];
            }
            if (isset($vals['defaults'])) {
                $this->defaults = $vals['defaults'];
            }
        }
    }

    public function getName()
    {
        return 'AccessGroupSpec';
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
                    if ($ftype == TType::I64) {
                        $xfer += $input->readI64($this->generation);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 3:
                    if ($ftype == TType::STRUCT) {
                        $this->options = new \Hypertable_ThriftGen\AccessGroupOptions();
                        $xfer += $this->options->read($input);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 4:
                    if ($ftype == TType::STRUCT) {
                        $this->defaults = new \Hypertable_ThriftGen\ColumnFamilyOptions();
                        $xfer += $this->defaults->read($input);
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
        $xfer += $output->writeStructBegin('AccessGroupSpec');
        if ($this->name !== null) {
            $xfer += $output->writeFieldBegin('name', TType::STRING, 1);
            $xfer += $output->writeString($this->name);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->generation !== null) {
            $xfer += $output->writeFieldBegin('generation', TType::I64, 2);
            $xfer += $output->writeI64($this->generation);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->options !== null) {
            if (!is_object($this->options)) {
                throw new TProtocolException('Bad type in structure.', TProtocolException::INVALID_DATA);
            }
            $xfer += $output->writeFieldBegin('options', TType::STRUCT, 3);
            $xfer += $this->options->write($output);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->defaults !== null) {
            if (!is_object($this->defaults)) {
                throw new TProtocolException('Bad type in structure.', TProtocolException::INVALID_DATA);
            }
            $xfer += $output->writeFieldBegin('defaults', TType::STRUCT, 4);
            $xfer += $this->defaults->write($output);
            $xfer += $output->writeFieldEnd();
        }
        $xfer += $output->writeFieldStop();
        $xfer += $output->writeStructEnd();
        return $xfer;
    }
}
