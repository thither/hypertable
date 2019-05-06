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

class ClientService_get_schema_str_with_ids_args
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
    );

    /**
     * @var int
     */
    public $ns = null;
    /**
     * @var string
     */
    public $table_name = null;

    public function __construct($vals = null)
    {
        if (is_array($vals)) {
            if (isset($vals['ns'])) {
                $this->ns = $vals['ns'];
            }
            if (isset($vals['table_name'])) {
                $this->table_name = $vals['table_name'];
            }
        }
    }

    public function getName()
    {
        return 'ClientService_get_schema_str_with_ids_args';
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
        $xfer += $output->writeStructBegin('ClientService_get_schema_str_with_ids_args');
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
        $xfer += $output->writeFieldStop();
        $xfer += $output->writeStructEnd();
        return $xfer;
    }
}
