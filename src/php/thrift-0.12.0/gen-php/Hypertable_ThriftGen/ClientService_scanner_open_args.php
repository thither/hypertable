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

class ClientService_scanner_open_args
{
    static public $isValidate = false;

    static public $_TSPEC = array(
        1 => array(
            'var' => 'ns',
            'isRequired' => false,
            'type' => TType::I64,
        ),
        2 => array(
            'var' => 'table_name',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
        3 => array(
            'var' => 'scan_spec',
            'isRequired' => false,
            'type' => TType::STRUCT,
            'class' => '\Hypertable_ThriftGen\ScanSpec',
        ),
    );

    /**
     * @var int
     */
    public $ns = null;
    /**
     * @var string
     */
    public $table_name = null;
    /**
     * @var \Hypertable_ThriftGen\ScanSpec
     */
    public $scan_spec = null;

    public function __construct($vals = null)
    {
        if (is_array($vals)) {
            if (isset($vals['ns'])) {
                $this->ns = $vals['ns'];
            }
            if (isset($vals['table_name'])) {
                $this->table_name = $vals['table_name'];
            }
            if (isset($vals['scan_spec'])) {
                $this->scan_spec = $vals['scan_spec'];
            }
        }
    }

    public function getName()
    {
        return 'ClientService_scanner_open_args';
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
                    if ($ftype == TType::I64) {
                        $xfer += $input->readI64($this->ns);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 2:
                    if ($ftype == TType::STRING) {
                        $xfer += $input->readString($this->table_name);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 3:
                    if ($ftype == TType::STRUCT) {
                        $this->scan_spec = new \Hypertable_ThriftGen\ScanSpec();
                        $xfer += $this->scan_spec->read($input);
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
        $xfer += $output->writeStructBegin('ClientService_scanner_open_args');
        if ($this->ns !== null) {
            $xfer += $output->writeFieldBegin('ns', TType::I64, 1);
            $xfer += $output->writeI64($this->ns);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->table_name !== null) {
            $xfer += $output->writeFieldBegin('table_name', TType::STRING, 2);
            $xfer += $output->writeString($this->table_name);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->scan_spec !== null) {
            if (!is_object($this->scan_spec)) {
                throw new TProtocolException('Bad type in structure.', TProtocolException::INVALID_DATA);
            }
            $xfer += $output->writeFieldBegin('scan_spec', TType::STRUCT, 3);
            $xfer += $this->scan_spec->write($output);
            $xfer += $output->writeFieldEnd();
        }
        $xfer += $output->writeFieldStop();
        $xfer += $output->writeStructEnd();
        return $xfer;
    }
}