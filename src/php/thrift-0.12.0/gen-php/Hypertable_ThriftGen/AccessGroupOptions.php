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

class AccessGroupOptions
{
    static public $isValidate = false;

    static public $_TSPEC = array(
        1 => array(
            'var' => 'replication',
            'isRequired' => false,
            'type' => TType::I16,
        ),
        2 => array(
            'var' => 'blocksize',
            'isRequired' => false,
            'type' => TType::I32,
        ),
        3 => array(
            'var' => 'compressor',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
        4 => array(
            'var' => 'bloom_filter',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
        5 => array(
            'var' => 'in_memory',
            'isRequired' => false,
            'type' => TType::BOOL,
        ),
    );

    /**
     * @var int
     */
    public $replication = null;
    /**
     * @var int
     */
    public $blocksize = null;
    /**
     * @var string
     */
    public $compressor = null;
    /**
     * @var string
     */
    public $bloom_filter = null;
    /**
     * @var bool
     */
    public $in_memory = null;

    public function __construct($vals = null)
    {
        if (is_array($vals)) {
            if (isset($vals['replication'])) {
                $this->replication = $vals['replication'];
            }
            if (isset($vals['blocksize'])) {
                $this->blocksize = $vals['blocksize'];
            }
            if (isset($vals['compressor'])) {
                $this->compressor = $vals['compressor'];
            }
            if (isset($vals['bloom_filter'])) {
                $this->bloom_filter = $vals['bloom_filter'];
            }
            if (isset($vals['in_memory'])) {
                $this->in_memory = $vals['in_memory'];
            }
        }
    }

    public function getName()
    {
        return 'AccessGroupOptions';
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
                    if ($ftype == TType::I16) {
                        $xfer += $input->readI16($this->replication);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 2:
                    if ($ftype == TType::I32) {
                        $xfer += $input->readI32($this->blocksize);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 3:
                    if ($ftype == TType::STRING) {
                        $xfer += $input->readString($this->compressor);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 4:
                    if ($ftype == TType::STRING) {
                        $xfer += $input->readString($this->bloom_filter);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 5:
                    if ($ftype == TType::BOOL) {
                        $xfer += $input->readBool($this->in_memory);
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
        $xfer += $output->writeStructBegin('AccessGroupOptions');
        if ($this->replication !== null) {
            $xfer += $output->writeFieldBegin('replication', TType::I16, 1);
            $xfer += $output->writeI16($this->replication);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->blocksize !== null) {
            $xfer += $output->writeFieldBegin('blocksize', TType::I32, 2);
            $xfer += $output->writeI32($this->blocksize);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->compressor !== null) {
            $xfer += $output->writeFieldBegin('compressor', TType::STRING, 3);
            $xfer += $output->writeString($this->compressor);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->bloom_filter !== null) {
            $xfer += $output->writeFieldBegin('bloom_filter', TType::STRING, 4);
            $xfer += $output->writeString($this->bloom_filter);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->in_memory !== null) {
            $xfer += $output->writeFieldBegin('in_memory', TType::BOOL, 5);
            $xfer += $output->writeBool($this->in_memory);
            $xfer += $output->writeFieldEnd();
        }
        $xfer += $output->writeFieldStop();
        $xfer += $output->writeStructEnd();
        return $xfer;
    }
}
